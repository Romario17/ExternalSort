#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define FILE_SIZE 59
#define MAX_ID 10000
#define PATH "StudentFile.dat"

void externalSort(const char *fileName, int fileSize, size_t sizeOfElements,
        int (*compare)(const void*, const void*), int internalMemorySize, int pathsNumber);
void fillFile();
void readFile();

int compare(const void *a, const void *b) {
    if (*(int *) a == *(int *) b)
        return 0;
    else if (*(int *) a < *(int *) b)
        return -1;
    else
        return 1;
}

int main(int argc, char** argv) {
    //Capacidade da memória interna (M registros)
    int internalMemorySize;
    //Tamanho do arquivo (N registros)
    int fileSize = (int) FILE_SIZE;
    //Número de discos utilizados em cada passada
    int pathsNumber;

    printf("Capacidade da memoria interna(M registros): ");
    scanf("%hu", &internalMemorySize);
    printf("Numero de discos disponiveis(N registros): ");
    scanf("%hu", &pathsNumber);

    // Fill file with a large amount of data
    fillFile();
    readFile();

    externalSort(PATH, fileSize, sizeof (int), compare, internalMemorySize, pathsNumber);
    
    printf("\n");
    readFile();

    return (EXIT_SUCCESS);
}

void createOrderedBlocks(FILE *inputDisk, FILE **outputDisks, size_t sizeOfElements,
        int (*compare)(const void*, const void*), int internalMemorySize, int pathsNumber) {

    void* recordsVector = malloc(sizeOfElements * internalMemorySize);
    int recordsRead = 0;
    int selectedDisk = 0;

    while (fread(recordsVector + (sizeOfElements * recordsRead), sizeOfElements, 1, inputDisk) == 1) {
        recordsRead++;
        if (recordsRead == internalMemorySize) {
            qsort(recordsVector, recordsRead, sizeOfElements, compare);
            for (int i = 0; i < recordsRead; i++) {
                fwrite(recordsVector + (sizeOfElements * i), sizeOfElements, 1, outputDisks[selectedDisk]);
            }
            if (selectedDisk == (pathsNumber - 1))
                selectedDisk = 0;
            else
                selectedDisk++;
            recordsRead = 0;
        }
    }

    if (recordsRead > 0) {
        qsort(recordsVector, recordsRead, sizeOfElements, compare);
        for (int i = 0; i < recordsRead; i++) {
            fwrite(recordsVector + (sizeOfElements * i), sizeOfElements, 1, outputDisks[selectedDisk]);
        }
    }
    free(recordsVector);
}

//======================================================================================

typedef struct readingDisk {
    int index;
    void* lastRecordAddr;
} ReadingDisk;

typedef struct heap {
    ReadingDisk* vector;
    int (*compare)(const void*, const void*);
    int size;
    int max;
} Heap;

Heap* heap_create(int (*compare)(const void*, const void*), int maxElements) {
    Heap *heap = (Heap*) malloc(sizeof (Heap));
    heap->vector = (ReadingDisk*) malloc(sizeof (ReadingDisk) * maxElements);
    heap->compare = compare;
    heap->size = 0;
    heap->max = maxElements;
    return heap;
}

void heap_moveUp(Heap* heap, int nodeIndex) {
    int fatherIndex = (nodeIndex - 1) / 2;

    if (nodeIndex > 0) {
        if (heap->compare(heap->vector[nodeIndex].lastRecordAddr,
                heap->vector[fatherIndex].lastRecordAddr) < 0) {
            //swap
            ReadingDisk aux = heap->vector[nodeIndex];
            heap->vector[nodeIndex] = heap->vector[fatherIndex];
            heap->vector[fatherIndex] = aux;

            heap_moveUp(heap, fatherIndex);
        }
    }
}

void heap_moveDown(Heap *heap, int nodeIndex) {
    int childIndex = (2 * nodeIndex) + 1;

    if (childIndex < heap->size) {
        if (childIndex < heap->size - 1) {
            if (heap->compare(heap->vector[childIndex + 1].lastRecordAddr,
                    heap->vector[childIndex].lastRecordAddr) < 0)
                childIndex = childIndex + 1;
        }
        if (heap->compare(heap->vector[nodeIndex].lastRecordAddr,
                heap->vector[childIndex].lastRecordAddr) > 0) {
            //swap
            ReadingDisk aux = heap->vector[nodeIndex];
            heap->vector[nodeIndex] = heap->vector[childIndex];
            heap->vector[childIndex] = aux;

            heap_moveDown(heap, childIndex);
        }
    }
}

bool heap_insert(Heap *heap, ReadingDisk donor) {
    if (heap->size == heap->max)
        return false;
    heap->vector[heap->size] = donor;
    heap_moveUp(heap, heap->size);
    heap->size++;
    return true;
}

bool heap_remove(Heap *heap, ReadingDisk* receiver) {
    if (heap->size == 0)
        return false;
    heap->size--;
    *receiver = heap->vector[0];
    heap->vector[0] = heap->vector[heap->size];
    heap_moveDown(heap, 0);
    return true;
}

void heap_destroy(Heap* heap) {
    free(heap->vector);
    free(heap);
}
//======================================================================================

void mergeOrderedBlocks(FILE **inputDisks, FILE *outputDisk, size_t sizeOfElements,
        int (*compare)(const void*, const void*), int internalMemorySize, int pathsNumber, int blockSize) {

    void* recordsRead = malloc(sizeOfElements * internalMemorySize);
    int* numRecordsRead = (int*) calloc(internalMemorySize, sizeof (int));

    ReadingDisk rd;
    Heap* heap = heap_create(compare, internalMemorySize);

    int diskIndex = 0;
    rd.index = diskIndex;
    rd.lastRecordAddr = recordsRead;

    //heap config
    for (int i = 0; i < internalMemorySize &&
            fread(rd.lastRecordAddr, sizeOfElements, 1, inputDisks[diskIndex]) == 1; i++) {
        heap_insert(heap, rd);
        numRecordsRead[diskIndex]++;
        if (diskIndex == pathsNumber - 1)
            diskIndex = 0;
        else
            diskIndex++;
        rd.index = diskIndex;
        rd.lastRecordAddr = recordsRead + (sizeOfElements * (i + 1));
    }

    rd.lastRecordAddr = NULL;

    while (heap_remove(heap, &rd)) {
        fwrite(rd.lastRecordAddr, sizeOfElements, 1, outputDisk);
        if (numRecordsRead[rd.index] != blockSize &&
                fread(rd.lastRecordAddr, sizeOfElements, 1, inputDisks[rd.index]) == 1) {
            numRecordsRead[rd.index]++;
            heap_insert(heap, rd);
        }
    }

    free(recordsRead);
    heap_destroy(heap);
}

void externalSort(const char *pathFile, int fileSize, size_t sizeOfElements,
        int (*compare)(const void*, const void*), int internalMemorySize, int pathsNumber) {
    
    int numberOfPasses = (int) ceil(log((double) fileSize / internalMemorySize) / log((double) pathsNumber));
    
    if(numberOfPasses < 0)
        return;
    
    FILE *file = fopen(pathFile, "rb+"); // open input file

    //create disks
    FILE** inputDisks = (FILE**) malloc(sizeof (FILE*) * pathsNumber);
    FILE** outputDisks = (FILE**) malloc(sizeof (FILE*) * pathsNumber);

    //first stage
    for (int i = 0; i < pathsNumber; i++) {
        inputDisks[i] = NULL;
        outputDisks[i] = tmpfile();
    }

    createOrderedBlocks(file, outputDisks, sizeOfElements, compare, internalMemorySize, pathsNumber);

    printf("\n===========================First Stage==============================\n");
    printf("Tamanho Bloco = %d || Numero de blocos = %d\n", internalMemorySize, (int) ceil(((double) fileSize / internalMemorySize) / pathsNumber));
    for (int i = 0; i < pathsNumber; i++) {
        rewind(outputDisks[i]);
        int num;
        printf("disco %d: ", i);
        while (fread(&num, sizeOfElements, 1, outputDisks[i]) == 1)
            printf("%d ", num);
        printf("\n");
    }
    printf("====================================================================\n");

    //second stage
    int blockSize = internalMemorySize;

    printf("\nNumber of passes %d\n", numberOfPasses);

    printf("\n===========================Second Stage==============================\n");
    for (int i = 0; i < numberOfPasses - 1; i++) {
        FILE** aux = inputDisks;
        inputDisks = outputDisks;
        outputDisks = aux;

        for (int i = 0; i < pathsNumber; i++) {
            rewind(inputDisks[i]);
            if (outputDisks[i] != NULL)
                fclose(outputDisks[i]);
            outputDisks[i] = tmpfile();
        }

        int diskIndex;
        int blocksNumber = (int) ceil(((double) fileSize / blockSize) / pathsNumber);
        for (int i = 0; i < blocksNumber; i++) {
            diskIndex = i % pathsNumber;
            mergeOrderedBlocks(inputDisks, outputDisks[diskIndex], sizeOfElements, compare,
                    internalMemorySize, pathsNumber, blockSize);
        }
        
        blockSize *= pathsNumber;
        
        //============================================================================
        printf("Passo: %d || Tamanho Bloco = %d || Numero de blocos = %d\n", i, blockSize, (int) ceil(((double) fileSize / blockSize) / pathsNumber));
        for (int i = 0; i < pathsNumber; i++) {
            rewind(outputDisks[i]);
            int num;
            printf("disco %d: ", i);
            while (fread(&num, sizeOfElements, 1, outputDisks[i]) == 1)
                printf("%d ", num);
            printf("\n");
        }
        printf("====================================================================\n");
    }

    for (int i = 0; i < pathsNumber; i++)
        rewind(outputDisks[i]);
    rewind(file);

    for (int i = 0; i < pathsNumber; i++)
        mergeOrderedBlocks(outputDisks, file, sizeOfElements, compare,
            internalMemorySize, pathsNumber, blockSize);

    for (int i = 0; i < pathsNumber; i++) {
        fclose(inputDisks[i]);
        fclose(outputDisks[i]);
    }

    free(inputDisks);
    free(outputDisks);

    fclose(file);
}

void fillFile() {
    FILE *studentFile = fopen(PATH, "wb");
    int num;

    /*Record aRecord;

            // These should instead go inside the loop, choosing different values each time
            // (But, for now, childIndexust use the same values for these two fields in all records,
            // varying only the randomly generated ID field.)
            strcpy(aRecord.course, "Computer Science");
            strcpy(aRecord.studentName, "childIndex. S. Sommers");
     */

    int vetor[FILE_SIZE];
    for (int i = 0; i < FILE_SIZE; i++) {
        vetor[i] = i;
    }

    srand(time(NULL));

    // Write data to file
    for (int i = 0; i < FILE_SIZE; i++) {
        //aRecord.studentID = rand();

        num = rand() % (FILE_SIZE - i);
        //printf("Student ID (created): %d\n", aRecord.studentID);
        //printf("%d\n", num);
        //fwrite(&aRecord, sizeof(aRecord), 1, studentFile);
        fwrite(&vetor[num], sizeof (int), 1, studentFile);

        vetor[num] = vetor[(FILE_SIZE - i) - 1];
    }

    fclose(studentFile);
}

void readFile() {
    FILE *studentFile = fopen(PATH, "rb");

    //Record aRecord;
    int num;

    for (int i = 0; i < FILE_SIZE; i++) {
        //fread(&aRecord, sizeof(aRecord), 1, studentFile);
        fread(&num, sizeof (int), 1, studentFile);
        //printf("Student ID: %d\n", aRecord.studentID);
        printf("%d ", num);
    }

    fclose(studentFile);
}