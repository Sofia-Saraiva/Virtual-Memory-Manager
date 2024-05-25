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
    int tlb_index;
    int frame_number;
    int time;
    int value; // value from backstore
    int valid; // if 1 is allocated, if 0 NOT
} Memory;

typedef struct {
    int page_number;
    int frame_number;
    int free; 
} Tlb;

typedef struct {
    int page_number;
    int free; // if 1 is free, if 0 is NOT free
    int time; // used to keep track of lru
} Physical_memory;

int binaryToDecimal(const char *binary); // binary/decimal conversion
char* decimalToBinary(int decimal);
int offsetToDecimal(const char *binary);
int pageToDecimal(const char *binary);

Memory* readFromFile(char *filename, int *number); // read files
char readBackingStore(int page_number, int offset);

void init_physical_memory(); // physical memory manipulation
void fifo_physical_memory(Memory *memory);
void lru_physical_memory(Memory *memory);
int find_free_frame();
int get_current_time();

void init_page_table(); // page table manipulation
void verify_page_table(Memory *memory);
void fifo_page_table(Memory *memory);
void lru_page_table(Memory *memory);
int find_free_page();
void handle_page_fault(Memory *memory);
void lru_page_table(Memory *memory);

void init_tlb(); // tlb manipulation
void verify_tlb(Memory *memory);
void handle_tlb(Memory *memory);
int find_free_tlb();

Tlb tlb[TLB_SIZE];
Memory page_table[NUMBER_OF_FRAMES];
Physical_memory physical_memory[NUMBER_OF_FRAMES];
int hit = 0;
int fault = 0;
int tlb_fifo_index = 0; 
int physical_index = 0;
char algorithm;

int main(int argc, char *argv[]) {
    int addresses;
    Memory *memory = readFromFile(argv[1], &addresses);
    if (memory == NULL) {
        return 1;
    }

    if (strcmp(argv[2], "fifo") == 0) {
        algorithm = 'F';
    }
    else if (strcmp(argv[2], "lru") == 0) {
        algorithm = 'L';
    }

    init_physical_memory();
    init_page_table();
    init_tlb();

    for (int i = 0; i < addresses; i++) {
        verify_tlb(&memory[i]);
    }

    // output
    FILE *output = fopen("correct.txt", "w");
    for (int i = 0; i < addresses; i++) {
        int physical = memory[i].page_offset + (memory[i].frame_number * FRAME_SIZE);
        printf("FRAME NUMBER %d\n", memory[i].frame_number);
        fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", memory[i].address, memory[i].tlb_index, physical, memory[i].value);
    }
    fprintf(output, "Number of Translated Addresses = %d\n", addresses);
    fprintf(output, "Page Faults = %d\n", fault);
    fprintf(output, "Page Fault Rate = %.3f\n", (float) fault / addresses);
    fprintf(output, "TLB Hits = %d\n", hit);
    fprintf(output, "TLB Hit Rate = %.3f\n", (float) hit / addresses);

    fclose(output);
    free(memory);

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
        physical_memory[i].time = -1;
    }
}

void init_page_table() {
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        page_table[i].valid = 0;
    }
}

void init_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].free = 1;
    }
}

void verify_tlb(Memory *memory) {
    int page_number = memory->page_number;
    int found = 0;
    for (int i = 0; i < TLB_SIZE; i++) { // checks if its in tlb
        if (page_number == tlb[i].page_number) {
            hit++;
            memory->tlb_index = i;
            memory->frame_number = tlb[i].frame_number;
            physical_memory[memory->frame_number].time = get_current_time();
            found = 1;
            break;
        }
    }

    if (!found) { // if not found in tlb, search in page table
        verify_page_table(memory);
    } 
}

void verify_page_table(Memory *memory) {
    int page_number = memory->page_number;
    int found = 0;
    for (int i = 0; i < NUMBER_OF_PAGES; i++) {
        if (page_table[i].valid == 1 && page_number == page_table[i].page_number) { // finds address on table
            found = 1;
            memory->frame_number = page_table[i].frame_number;
            physical_memory[memory->frame_number].time = get_current_time();
            break;
        }
    }

    if (found) {
        handle_tlb(memory);
        return;
    } else { 
        handle_page_fault(memory);
    }
}
void fifo_physical_memory(Memory *memory) {
    physical_memory[physical_index].page_number = memory->page_number;
    physical_memory[physical_index].free = 0;
    memory->frame_number = physical_index;

    physical_index = (physical_index + 1) % NUMBER_OF_FRAMES;
}

void fifo_page_table(Memory *memory) {
    for (int i = 0; i < NUMBER_OF_PAGES - 1; i++) {
        page_table[i] = page_table[i + 1];
    }

    page_table[NUMBER_OF_PAGES - 1].page_number = memory->page_number;
    page_table[NUMBER_OF_PAGES - 1].frame_number = memory->frame_number;
    page_table[NUMBER_OF_PAGES - 1].valid = 1;
}

void fifo_tlb(Memory *memory) {
    tlb[tlb_fifo_index].page_number = memory->page_number;
    tlb[tlb_fifo_index].frame_number = memory->frame_number;
    tlb[tlb_fifo_index].free = 0;
    memory->tlb_index = tlb_fifo_index;

    tlb_fifo_index = (tlb_fifo_index + 1) % TLB_SIZE;
}

void lru_physical_memory(Memory *memory) {
    int index = 0;
    int least_page = physical_memory[0].time;

    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        if (physical_memory[i].time < least_page) {
            least_page = physical_memory[i].time;
            index = i;
        }
    }

    physical_memory[index].time = get_current_time();
    physical_memory[index].page_number = memory->page_number; 
    physical_memory[index].free = 0;
    memory->frame_number = index;
}

void lru_page_table(Memory *memory) {
    int index = 0;
    int least_page = physical_memory[0].time;

    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        if (physical_memory[i].time < least_page) {
            least_page = physical_memory[i].time;
            index = i;
        }
    }

    page_table[index].page_number = memory->page_number; 
    page_table[index].frame_number = memory->frame_number;
    page_table[index].valid = 1;
}

void handle_page_fault(Memory *memory) { // update physical memory and page table
    fault++;

    int index = find_free_frame();  // if free, allocate in free frame
    if (index != -1) {
        physical_memory[index].page_number = memory->page_number;
        physical_memory[index].free = 0;
        memory->frame_number = index;

        int index_page = find_free_page();
        if (index != -1) {
            page_table[index_page].valid = 1;
            page_table[index_page].page_number = memory->page_number;
            page_table[index_page].frame_number = memory->frame_number;
        }
    } 
    else { // if not free, use fifo or lru
        if (algorithm == 'F') {
            fifo_physical_memory(memory);
            fifo_page_table(memory);
        }
        else if (algorithm == 'L') {
            lru_physical_memory(memory);
            lru_page_table(memory);
        }
    }
    handle_tlb(memory);
}

void handle_tlb(Memory *memory) { // update tlb values
    int index_tlb = find_free_tlb();
    if (index_tlb != -1) { // if tlb is free, allocate in tlb
        memory->tlb_index = index_tlb;
        tlb[index_tlb].page_number = memory->page_number;
        tlb[index_tlb].frame_number = memory->frame_number;
        tlb[index_tlb].free = 0;
    }
    else { // if not, fifo in tlb
        fifo_tlb(memory);
    }
}

int find_free_frame() {
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        if (physical_memory[i].free == 1) {
            return i; 
        }
    }
    return -1; 
}

int find_free_page() {
    for (int i = 0; i < NUMBER_OF_PAGES; i++) {
        if (page_table[i].valid == 0) {
            return i; 
        }
    }
    return -1; 
}

int find_free_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].free == 1) {
            return i;
        }
    }
    return -1;
}

int get_current_time() {
    int current = 0;
    for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
        if (physical_memory[i].time > current) {
            current = physical_memory[i].time;
        }
    }

    return current + 1;
}