#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME 128

typedef enum {
    slip,
    land,
    trailor,
    storage,
    no_place
} PlaceType;

PlaceType StringToPlaceType(char *PlaceString) {
    if (!strcasecmp(PlaceString, "slip")) return slip;
    if (!strcasecmp(PlaceString, "land")) return land;
    if (!strcasecmp(PlaceString, "trailor")) return trailor;
    if (!strcasecmp(PlaceString, "storage")) return storage;
    return no_place;
}

char *PlaceToString(PlaceType place) {
    switch (place) {
        case slip: return "slip";
        case land: return "land";
        case trailor: return "trailor";
        case storage: return "storage";
        case no_place: return "no_place";
        default:
            printf("Invalid place\n");
            exit(EXIT_FAILURE);
    }
}

typedef union {
    int slip_number;
    char bay_letter;
    char license_tag[16];
    int storage_number;
} LocationInfo;

typedef struct {
    char name[MAX_NAME];
    float length;
    PlaceType place;
    LocationInfo location;
    float amount_owed;
} Boat;

Boat *boats[MAX_BOATS];
int boat_count = 0;

float GetMonthlyRate(PlaceType place) {
    switch (place) {
        case slip: return 12.50;
        case land: return 14.00;
        case trailor: return 25.00;
        case storage: return 11.20;
        default: return 0;
    }
}

void ToLowerStr(char *s) {
    while (*s) {
        *s = tolower(*s);
        s++;
    }
}

int FindBoatIndexByName(const char *name) {
    for (int i = 0; i < boat_count; i++) {
        if (strcasecmp(name, boats[i]->name) == 0)
            return i;
    }
    return -1;
}

void PrintBoat(const Boat *b) {
    printf("%-20s %3.0f' %8s ", b->name, b->length, 
PlaceToString(b->place));
    switch (b->place) {
        case slip:
            printf("  # %2d", b->location.slip_number);
            break;
        case land:
            printf("     %c ", b->location.bay_letter);
            break;
        case trailor:
            printf("%7s", b->location.license_tag);
            break;
        case storage:
            printf("  # %2d", b->location.storage_number);
            break;
        default:
            printf("Unknown");
    }
    printf("   Owes $%7.2f\n", b->amount_owed);
}

void PrintInventory() {
    for (int i = 0; i < boat_count; i++) {
        PrintBoat(boats[i]);
    }
}

void AddBoatFromCSV(char *line) {
    if (boat_count >= MAX_BOATS) {
        printf("Boat list is full.\n");
        return;
    }

    Boat *b = malloc(sizeof(Boat));
    char place_str[32], extra_info[32];
    sscanf(line, "%127[^,],%f,%31[^,],%31[^,],%f", b->name, &b->length, 
place_str, extra_info, &b->amount_owed);

    b->place = StringToPlaceType(place_str);

    switch (b->place) {
        case slip:
            b->location.slip_number = atoi(extra_info); break;
        case land:
            b->location.bay_letter = extra_info[0]; break;
        case trailor:
            strncpy(b->location.license_tag, extra_info, 15); break;
        case storage:
            b->location.storage_number = atoi(extra_info); break;
        default:
            printf("Invalid place type\n");
            free(b);
            return;
    }

    int i = boat_count;
    while (i > 0 && strcasecmp(boats[i - 1]->name, b->name) > 0) {
        boats[i] = boats[i - 1];
        i--;
    }
    boats[i] = b;
    boat_count++;
}

void RemoveBoat(const char *name) {
    int idx = FindBoatIndexByName(name);
    if (idx == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[idx]);
    for (int i = idx; i < boat_count - 1; i++) {
        boats[i] = boats[i + 1];
    }
    boat_count--;
}

void AcceptPayment(const char *name, float amount) {
    int idx = FindBoatIndexByName(name);
    if (idx == -1) {
        printf("No boat with that name\n");
        return;
    }
    if (amount > boats[idx]->amount_owed) {
        printf("That is more than the amount owed, $%.2f\n", 
boats[idx]->amount_owed);
        return;
    }
    boats[idx]->amount_owed -= amount;
}

void UpdateForNewMonth() {
    for (int i = 0; i < boat_count; i++) {
        boats[i]->amount_owed += boats[i]->length * 
GetMonthlyRate(boats[i]->place);
    }
}

void SaveDataToFile(const char *filename) {
    FILE *fp = fopen(filename, "w");
    for (int i = 0; i < boat_count; i++) {
        Boat *b = boats[i];
        fprintf(fp, "%s,%.0f,%s,", b->name, b->length, 
PlaceToString(b->place));
        switch (b->place) {
            case slip: fprintf(fp, "%d", b->location.slip_number); break;
            case land: fprintf(fp, "%c", b->location.bay_letter); break;
            case trailor: fprintf(fp, "%s", b->location.license_tag); 
break;
            case storage: fprintf(fp, "%d", b->location.storage_number); 
break;
            default: break;
        }
        fprintf(fp, ",%.2f\n", b->amount_owed);
    }
    fclose(fp);
}

void LoadDataFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        AddBoatFromCSV(line);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    LoadDataFromFile(argv[1]);

    printf("Welcome to the Boat Management System\n");

    char option;
    char input[256];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(input, sizeof(input), stdin);
        option = tolower(input[0]);

        switch (option) {
            case 'i':
                PrintInventory();
                break;
            case 'a':
                printf("Please enter the boat data in CSV format         : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                AddBoatFromCSV(input);
                break;
            case 'r':
                printf("Please enter the boat name                    : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                RemoveBoat(input);
                break;
            case 'p':
                printf("Please enter the boat name           : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                char name[128];
                strcpy(name, input);
                printf("Please enter the amount to be paid                : ");
                fgets(input, sizeof(input), stdin);
                float amt = atof(input);
                AcceptPayment(name, amt);
                break;
            case 'm':
                UpdateForNewMonth();
                break;
            case 'x':
                SaveDataToFile(argv[1]);
                printf("Exiting the Boat Management System\n");
                return 0;
            default:
                printf("Invalid option %c\n", option);
        }
    }

    return 0;
}

