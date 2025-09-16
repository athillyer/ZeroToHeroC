#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>

#include "file.h"
#include "parse.h"
#include "common.h"

void print_usage(char *argv[])
{
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n -create new database file\n");
    printf("\t -f - (required) path to database file\n");
}

int main(int argc, char *argv[])
{
    char* filepath = NULL;
    char* addString = NULL;
    //Employee's name to be removed
    char* removeString = NULL;
    //Employee's name to be updated, prompt user for hours? Or get it on the same line?
    char* updateString = NULL;
    bool newfile = false;
    bool list = false;
    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    int c = 0;
    //when it has a colon in front of it, that means it needs a parameter
    while ((c = getopt(argc, argv, "nf:a:r:u:l")) != -1){
        switch(c){
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'a':
                addString = optarg;
                break;
            case 'l':
                list = true;
                break;
            case 'r':
                removeString = optarg;
                break;
            case 'u':
                updateString = optarg;
                break;
            case '?':
                //getopt returns ? when it gets a character not in the list
                printf("Unknown option -%c\n", c);
                break;
            default:
                return -1;
        }
    }

    if(filepath == NULL){
        printf("Filepath is a required argument\n");
        print_usage(argv);
        
        return 0;
    }

    if(newfile)
    {
        dbfd = create_db_file(filepath);
        if(dbfd == STATUS_ERROR)
        {
            printf("Unable to create database file\n");
            return -1;
        }

        if(create_db_header(&dbhdr) == STATUS_ERROR)
        {
            printf("Failed to create database header\n");
            return -1;
        }

    }
    else
    {
        dbfd = open_db_file(filepath);
        if(dbfd == STATUS_ERROR)
        {
            printf("Unable to open database file\n");
            return -1;
        }

        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR)
        {
            printf("Failed to validate database header\n");
            return -1;
        }
    }

    if(read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS)
    {
        printf("Failed to read employees\n");
        return -1;
    }

    if(addString)
    {
        dbhdr->count++;
        employees = realloc(employees, sizeof(struct employee_t) * dbhdr->count);
        if(employees == NULL)
        {
            perror("realloc");
            return -1;
        }

        if(add_employee(dbhdr, employees, addString) != STATUS_SUCCESS)
        {
            printf("Failed to add employee\n");
            return -1;
        }
    }

    if(removeString)
    {
        //we'll change the dbhdr count in the method, as we might not see the employee
        if(remove_employee(dbhdr, employees, removeString) != STATUS_SUCCESS)
        {
            printf("Failed to remove employee\n");
            return -1;
        }
    }

    if(updateString)
    {
        if(update_employee(dbhdr, employees, updateString) != STATUS_SUCCESS)
        {
            printf("Failed to update employee\n");
            return -1;
        }
    }

    if(list)
    {
        list_employees(dbhdr, employees);
    }

    output_file(dbfd, dbhdr, employees);
    close(dbfd);

    return 0;
}