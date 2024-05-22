#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define MEMORY_SIZE 128 * 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define NUMBER_OF_FRAMES 128
#define NUMBER_OF_PAGES 128

typedef struct {
    int address;
    int page_number;
    int page_offset;
    int value;
    int valid; // if 1 is valid, if 0 is NOT valid
} Memory;

typedef struct {
    int page_number;
    int free; // if 1 is free, if 0 is NOT free
    char value;
} Physical_memory;

Memory tlb[TLB_SIZE];
Memory page_table[NUMBER_OF_FRAMES];
Physical_memory physical_memory[NUMBER_OF_FRAMES];
int hit = 0;
int fault = 0;

int binaryToDecimal(const char *binary);
char* decimalToBinary(int decimal);
int offsetToDecimal(const char *binary);
int pageToDecimal(const char *binary);

Memory* readFromFile(char *filename, int *number);
char readBackingStore(int page_number, int offset);

void init_physical_memory();
void init_page_table();
void verify_page_table(int page_number);
void fifo_physical_memory(int page_number);
void fifo_page_table(int page_number);
void handle_page_fault(int page_number);
int find_free_frame();

int main(int argc, char *argv[]) {
    int addresses;
    Memory *memory = readFromFile(argv[1], &addresses);
    if (memory == NULL) {
        return 1;
    }

    printf("starting!\n");
    init_physical_memory();
    init_page_table();

    for (int i = 0; i < addresses; i++) {
        verify_page_table(memory[i].page_number);
        for (int j = 0; j < NUMBER_OF_FRAMES; j++) {
            //printf("%d Página %d\n", j, page_table[j].page_number);
            //printf("%d Frame %d\n", j, physical_memory[j].page_number);
        }

        //int tlb;
        //int physical = memory[i].page_offset + (tlb * FRAME_SIZE);
        //printf("Virtual address: %d TLB: %d Physical address: %d Value: %d\n", memory[i].address, tlb_number, physical, memory[i].value);
    }

    // output
    printf("Number of Translated Addresses = %d\n", addresses);
    printf("Page Faults = %d\n", fault);
    printf("Page Fault Rate = %.3f\n", (float) fault / addresses);
    printf("TLB Hits = %d\n", hit);
    printf("TLB Hit Rate = %.3f\n", (float) hit / addresses);

    free(memory);
    printf("ending!\n");
    return 0;
}

int binaryToDecimal(const char *binary) {
    int decimal = 0;
    int len = strlen(binary);

    for (int i = 0; i < len; i++) {
        if (binary[i] == '1') {
            decimal = decimal * 2 + 1;
        } else {
            decimal = decimal * 2;
        }
    }

    return decimal;
}

char* decimalToBinary(int decimal) {
    if (decimal == 0) {
        char* binary = (char*)malloc(17); 
        binary[0] = '0';
        binary[1] = '\0';
        return binary;
    }

    int temp = decimal;
    int bitCount = 0;
    while (temp > 0) {
        temp /= 2;
        bitCount++;
    }

    int minBits = 16;
    if (bitCount > minBits) minBits = bitCount;

    char* binary = (char*)malloc(minBits + 1);
    int index = 0;

    while (decimal > 0) {
        binary[index++] = (decimal % 2) ? '1' : '0';
        decimal /= 2;
    }

    while (index < minBits) {
        binary[index++] = '0';
    }

    binary[index] = '\0';

    for (int i = 0, j = index - 1; i < j; i++, j--) {
        char temp = binary[i];
        binary[i] = binary[j];
        binary[j] = temp;
    }

    return binary;
}

int offsetToDecimal(const char *binary) {
    int len = strlen(binary);
    int inicio = (len > 8) ? (len - 8) : 0; 
    char *offset = (char*)malloc(9); 

    for (int i = inicio, j = 0; i < len; i++, j++) {
        offset[j] = binary[i];
    }

    int decOffset = binaryToDecimal(offset);
    free(offset);
    return decOffset;
}

int pageToDecimal(const char* binary) {
    char *page = (char*)malloc(9); 

    for (int i = 0; i < 8; i++) {
        page[i] = binary[i];
    }

    int decPage = binaryToDecimal(page);
    free(page);
    return decPage;
}

Memory* readFromFile(char *filename, int *number) {
    FILE *input = fopen(filename, "r");
    if (input == NULL) {
        return NULL;
    }

    Memory *memory = NULL;
    *number = 0;

    const int INITIAL_SIZE = 10;
    memory = (Memory *)malloc(INITIAL_SIZE * sizeof(Memory));
    if (memory == NULL) {
        fclose(input);
        return NULL;
    }

    int address;
    while (fscanf(input, "%d", &address) == 1) {
        if (*number >= INITIAL_SIZE) {
            Memory *temp = (Memory *)realloc(memory, (*number + 1) * sizeof(Memory));
            if (temp == NULL) {
                fclose(input);
                free(memory);
                return NULL;
            }
            memory = temp;
        }

        memory[*number].address = address;

        char *binary_address = decimalToBinary(address);
        memory[*number].page_offset = offsetToDecimal(binary_address);
        memory[*number].page_number = pageToDecimal(binary_address);
        memory[*number].value = readBackingStore(memory[*number].page_number, memory[*number].page_offset);

        free(binary_address);

        (*number)++;
    }

    fclose(input);
    return memory;
}

char readBackingStore(int page_number, int offset) {
    FILE *file = fopen("BACKING_STORE.bin", "rb");
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    int address = page_number * PAGE_SIZE + offset;

    if (fseek(file, address, SEEK_SET) != 0) {
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char value;
    if (fread(&value, sizeof(char), 1, file) != 1) {
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);

    return value;
}

void init_physical_memory() {
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) { 
        physical_memory[i].free = 1;
    }
}

void init_page_table() {
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        page_table[i].valid = 0;
    }
}

void verify_page_table(int page_number) {
    int found = 0;
    for (int i = 0; i < NUMBER_OF_PAGES; i++) {
        if (page_table[i].valid == 0 && page_number == page_table[i].page_number) { // finds address on table
            hit++;
            printf("Página %d já está alocada.\n", page_number);
            found = 1;
            break;
        }
    }

    if (found == 1) {
        return;
    } else {
        handle_page_fault(page_number);
    }
    
}
void fifo_physical_memory(int page_number) {
    for (int i = 0; i < NUMBER_OF_FRAMES - 1; i++) {
        physical_memory[i] = physical_memory[i + 1];
    }

    physical_memory[NUMBER_OF_FRAMES - 1].page_number = page_number;
    physical_memory[NUMBER_OF_FRAMES - 1].free = 0;  
}

void fifo_page_table(int page_number) {
    for (int i = 0; i < NUMBER_OF_PAGES - 1; i++) {
        page_table[i] = page_table[i + 1];
    }

    page_table[NUMBER_OF_PAGES - 1].page_number = page_number;
}

void handle_page_fault(int page_number) {
    fault++;
    int index = find_free_frame();  
    if (index != -1) {
        physical_memory[index].page_number = page_number;
        physical_memory[index].free = 0;
        page_table[index].page_number = page_number;
        page_table[index].valid = 0;
    }
 
    fifo_physical_memory(page_number);
    fifo_page_table(page_number);
}

int find_free_frame() {
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        if (physical_memory[i].free == 1) {
            return i; 
        }
    }
    return -1; 
}