#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees)
{
    for(int i = 0; i < dbhdr->count; i++)
    {
        printf("Employee %d\n", i + 1);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring)
{
    //assumes you've allocated space for the employee in the employee array
	char *name = strtok(addstring, ",");

	char *addr = strtok(NULL, ",");

	char *hours = strtok(NULL, ",");

	strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
	strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));

	employees[dbhdr->count-1].hours = atoi(hours);

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut)
{
    if(fd < 0)
    {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count; //assumes you used ntohl in a previous method

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if(employees == NULL)
    {
        perror("calloc");
        return STATUS_ERROR;
    }

    read(fd, employees, count*sizeof(struct employee_t));
    for(int i = 0; i < count; i++)
    {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

	int count = dbhdr->count;

    size_t newfileSize = sizeof(struct dbheader_t) + (sizeof(struct employee_t) * count);
    if(ftruncate(fd, newfileSize) == -1)
    {
        perror("ftruncate");
        return STATUS_ERROR;
    }

	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(newfileSize);
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

    //set position to beginning
	lseek(fd, 0, SEEK_SET);

	write(fd, dbhdr, sizeof(struct dbheader_t));

    //shouldn't write anything??
	for (int i = 0; i < count; i++) {
		employees[i].hours = htonl(employees[i].hours);
		write(fd, &employees[i], sizeof(struct employee_t));
	}

	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut)
{
    if(fd < 0)
    {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if(header == NULL)
    {
        perror("calloc");
        return STATUS_ERROR;
    }

    if(read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t))
    {
        perror("read");
        free(header);
        header = NULL;
        return STATUS_ERROR;
    }

    //ntohs -> network to host short
    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if(header->version != 1)
    {
        printf("Improper header version\n");
        free(header);
        header = NULL;
        return STATUS_ERROR;
    }

    if(header->magic != HEADER_MAGIC)
    {
        printf("Improper header magic\n");
        free(header);
        header = NULL;
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if(header->filesize != dbstat.st_size)
    {
        printf("Corrupted database\n");
        free(header);
        header = NULL;
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut)
{
    if(fd < 0)
    {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if(header == NULL)
    {
        perror("calloc");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
    return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *removeString)
{
    int count = dbhdr->count;
    for(int i = 0; i < count; i++)
    {
        if(strcasecmp(employees[i].name, removeString) == 0)
        {
            dbhdr->count--;
            printf("dbhdr count now %d\n", dbhdr->count);
            if(dbhdr->count == 0)
            {
                free(employees);
                employees == NULL;
                return STATUS_SUCCESS;
            }

            for (int j = i; j < count - 1; j++)
            {
                employees[j] = employees[j + 1];
            }

            struct employee_t *temp = realloc(employees, dbhdr->count * sizeof(struct employee_t));
            if(temp == NULL && dbhdr->count > 0) {
                perror("realloc");
                return STATUS_ERROR;
            }

            dbhdr->filesize = sizeof(struct dbheader_t) + (dbhdr->count * sizeof(struct employee_t));
            return STATUS_SUCCESS;
        }
    }

    //employee not found, no worries.
    printf("Employee not found. No employees removed.\n");
    return STATUS_SUCCESS;
}

int update_employee(struct dbheader_t * dbhdr, struct employee_t *employees, char *updateString)
{
    int count = dbhdr->count;
    char *name = strtok(updateString, ",");
    if(name == NULL)
    {
        printf("Cannot update employee without name.\n");
        return STATUS_ERROR;
    }
    char *hours = strtok(NULL, ",");
    if(hours == NULL)
    {
        printf("Cannot update employee hours with NULL.\n");
        return STATUS_ERROR;
    }

    int hoursInt = atoi(hours);

    for(int i = 0; i < count; i++)
    {
        if(strcasecmp(employees[i].name, name) == 0)
        {
            //only update this for one person.
            employees[i].hours = hoursInt;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_SUCCESS;
}