#define _CRT_SECURE_NO_WARNINGS
#define DelaysFlag 1
#define readThreadDelay 60
#define writeThreadDelay 10
#include <Windows.h>
#include <iostream>
#include <string>
#include "pthread.h"
#include "sched.h"
#include <chrono>
#include "semaphore.h"
#include <mutex>
#include <queue>
#pragma comment(lib, "pthreadVCE2.lib")
using namespace std;

/* рпк1 - модель с равноправными узлами, представляющими собой пары потоков,
первый читает входной файл и вычисляет результаты, второй записывает выходной файл.
Файл результатов общий, каждый результат записывается отдельно
(каждое обращение к файлу записывает один результат).
*/
//Объявление функций
void* ReadAndSolving(void* args);
void* Write(void* args);
bool isPrime(int n);

//Объявление глобальных переменных
std::mutex inputMutex;
int inputRowCount;
int maxRowCount;
FILE* inputF;

std::mutex outputMutex;
FILE* outputF;

//Функции
FILE* OpenFileR()
{
    FILE* f = fopen("input.txt", "r");
    if (f == NULL)
    {
        throw new exception("Файла input.txt нет!");
    }
    return f;
}
FILE* OpenFileW()
{
    FILE* f = fopen("output.txt", "w");
    if (f == NULL)
    {
        throw new exception("Файла output.txt нет!");
    }
    return f;
}
int GetNumberOfLines(FILE* f)
{
    rewind(f);
    int num = 0;
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];
    while (fgets(buffer, MAX_LENGTH, f))
        num++;
    rewind(f);
    return num;
}

struct Solution
{
    int lineNumber;
    int number;
    bool isPrime;
};
class Node
{
public:
    
    bool isFinished;
    std::mutex isFinishedMutex;

    queue<Solution> solutions;
    std::mutex queueMutex;

    pthread_t inputAndSolutionThread;
    pthread_t outputThread;

    Node()
    {
        isFinished = false;
    }
    void StartSolving()
    {
        //InputAndSolution(this);
        //Output(this);
        pthread_create(&inputAndSolutionThread, NULL, ReadAndSolving, this);
        pthread_create(&outputThread, NULL, Write, this);
    }
    void WaitSolving()
    {
        pthread_join(inputAndSolutionThread, NULL);
        pthread_join(outputThread, NULL);
    }
};

static long long ReadFile(FILE* f, int row)
{
    rewind(f);
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];
    int i = 0;
    while (fgets(buffer, MAX_LENGTH, f))
    {
        if (i == row)
        {
            try
            {
                int num = (int)_atoi64(buffer);
                return num;
            }
            catch (std::invalid_argument e)
            {
                return -1;
            }
            break;
        }
        i++;
    }
    rewind(f);
}
static void WriteFile(FILE* f, Solution solution)
{
    std::string str;
    str += std::to_string(solution.lineNumber + 1);
    str += ". ";
    str += std::to_string(solution.number);
    str += " - ";
    if (solution.isPrime)
        str += "простое число\n";
    else
        str += "не простое число\n";
    fwrite(str.c_str(), sizeof(char), str.size(), f);
}

void* ReadAndSolving(void* args)
{
    Node* node = (Node*)args;
    do
    {
        inputMutex.lock();
        if (inputRowCount >= maxRowCount)
        {
            node->isFinishedMutex.lock();
            node->isFinished = true;
            node->isFinishedMutex.unlock();
            inputMutex.unlock();
            return (void*)1;
        }
        int row = inputRowCount;
        int num = ReadFile(inputF, row);
        inputRowCount++;
        inputMutex.unlock();
        Solution solution = { row, num, isPrime(num) };
#if DelaysFlag Задержка
        Sleep(readThreadDelay);
#endif 
        node->queueMutex.lock();
        node->solutions.push(solution);
        node->queueMutex.unlock();
    } while (true);
}
void* Write(void* args)
{
    Node* node = (Node*)args;
    do
    {
        node->queueMutex.lock();
        if (node->solutions.size() == 0)
        {
            node->isFinishedMutex.lock();
            bool isFinished = node->isFinished;
            node->isFinishedMutex.unlock();
            node->queueMutex.unlock();
            if (isFinished)
                return (void*)1;
        }
        else
        {
            Solution solution = node->solutions.front();
            node->solutions.pop();
            node->queueMutex.unlock();
            outputMutex.lock();
            WriteFile(outputF, solution);
#if DelaysFlag Задержка
            Sleep(writeThreadDelay);
#endif 
            outputMutex.unlock();
        }
    } while (true);
}

bool isPrime(int n)
{
    for (long long i = 2; i <= sqrt(n); i++)
        if (n % i == 0)
            return false;
    return true;
}

int PrimeNumberSolver(int nodesCount = 1)
{
    if (nodesCount < 1)
        throw new exception("Кол-во узлов меньше 1");

    inputF = OpenFileR();
    outputF = OpenFileW();

    inputRowCount = 0;
    maxRowCount = GetNumberOfLines(inputF);

    Node **nodes = new Node*[nodesCount];

    for (size_t i = 0; i < nodesCount; i++) //инициализация узлов
    {
        Node *node = new Node;
        nodes[i] = node;
    }
    for (size_t i = 0; i < nodesCount; i++) //запуск потоков
        nodes[i]->StartSolving();
    auto start = std::chrono::high_resolution_clock::now(); //Запуск таймера
    for (int i = 0; i < nodesCount; i++) //Ожидаем выполнения
    {
        nodes[i]->WaitSolving();
    }
    auto elapsed = std::chrono::high_resolution_clock::now() - start; //Стоп таймера
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(); //Время выполнения
    for (size_t i = 0; i < nodesCount; i++)
        delete(nodes[i]);
    delete[](nodes);

    return milliseconds;
}

int main() {
	setlocale(LC_ALL, "RU");
    for (size_t i = 1; i <= 10; i++)
        std::cout << "Потоков: " << i << ". Время: " << PrimeNumberSolver(i) << std::endl;
	return 0;
}