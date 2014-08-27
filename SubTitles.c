#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "hashmap.h"

void free_matrix(char **c, int n);

char* substring(const char* str, size_t begin, size_t len)
{
    return strndup(str + begin, len);
}

HashMap *InsertSubsToHashTable(char *fullFileNamePath, int allSubsLinesLength)
{
    int i, endOfLinePos = 0, line_number = 0, firstTextLineIndex = 0;
    char line_buffer[BUFSIZ]; /* BUFSIZ is defined if you include stdio.h */
    FILE *infile;
    char endOfLine[] = "\n";
    HashMap *hm;
    char *shours, *sminutes, *sseconds, *smiliSeconds, scurTotlaTime[100];
    char *ehours, *eminutes, *eseconds, *emiliSeconds, ecurTotlaTime[100];
    scurTotlaTime[0] = 0;
    ecurTotlaTime[0] = 0;
    char subLines[(BUFSIZ * 2) + 50], **strArr;
    
    infile = fopen(fullFileNamePath, "r");
    
    if(!infile)
    {
        g_error("Couldn't' open file %s for reading", fullFileNamePath);
    }

    //Count number of subtitles in .srt files
    while(fgets(line_buffer, sizeof(line_buffer), infile))
    {
        ++line_number;
    }
    
    fclose(infile);
    hm = hm_new(line_number);
    
    if (hm == NULL)
    {
        g_error("Could'nt allocate hash table");
    }

    int lineIndex = 0;
    infile = fopen(fullFileNamePath, "r");
    
    if(!infile)
    {
        g_error("Couldn't' open file %s for reading", fullFileNamePath);
    }

    strArr = (char**)malloc((line_number + 1) * sizeof(char*));
    
    for(i = 0; i < line_number + 1; i++)
    {
        strArr[i] = (char*)malloc((BUFSIZ + 1) * sizeof(char));
	}
	
    while(fgets(line_buffer, sizeof(line_buffer), infile))
    {
        strcpy(strArr[lineIndex], line_buffer);

        lineIndex++;
    }
    
    fclose(infile);

    for(i = 0; i < lineIndex; i++)
    {
        endOfLinePos = strcspn(strArr[i], endOfLine) + 1;
        
        if(endOfLinePos > 8)
        {
            if(strcmp(substring(strArr[i], 2, 1), ":") == 0)
            {
                shours = substring(strArr[i], 0, 2);
                sminutes = substring(strArr[i], 3, 2);
                sseconds = substring(strArr[i], 6, 2);
                smiliSeconds = substring(strArr[i], 9, 2);
                firstTextLineIndex = i;
                strcat(scurTotlaTime, shours);
                strcat(scurTotlaTime, ":");
                strcat(scurTotlaTime, sminutes);
                strcat(scurTotlaTime, ":");
                strcat(scurTotlaTime, sseconds);
                strcat(scurTotlaTime, ":");
                strcat(scurTotlaTime, smiliSeconds);

                if(strcmp(substring(strArr[i], 13, 3), "-->") == 0)
                {
                    ehours = substring(strArr[i], 13, 6);
                    eminutes = substring(strArr[i], 20, 2);
                    eseconds = substring(strArr[i], 23, 2);
                    emiliSeconds = substring(strArr[i], 26, 2);

                    strcat(ecurTotlaTime, ehours);
                    strcat(ecurTotlaTime, ":");
                    strcat(ecurTotlaTime, eminutes);
                    strcat(ecurTotlaTime, ":");
                    strcat(ecurTotlaTime, eseconds);
                    strcat(ecurTotlaTime, ":");
                    strcat(ecurTotlaTime, emiliSeconds);
                    hm_put(hm, ecurTotlaTime, ""); //insert end time
                }

                strcat(subLines, strArr[firstTextLineIndex + 1]);
                strcat(subLines, strArr[firstTextLineIndex + 2]);
                hm_put(hm, scurTotlaTime, subLines); //insert start time                
                strcpy(scurTotlaTime, ""); //clean string
                strcpy(ecurTotlaTime, ""); //clean string
                strcpy(subLines, ""); //clean string
            }
        }
    }
    
    free(shours);
    free(sminutes);
    free(sseconds);
    free(smiliSeconds);
    free(ehours);
    free(eminutes);
    free(eseconds);
    free(emiliSeconds);
    free_matrix(strArr, line_number - 1);
    return hm;
}

void free_matrix(char **c, int n)
{
    int i;
    
    for(i = 0; i < n; i++)
    {
        free(c[i]);
	}
	
    free(c);
}
