#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include "pthread.h"
#include "sched.h"
#include "semaphore.h"
#include <mutex>
#include <queue>
#include <list>
#pragma comment(lib, "pthreadVCE2.lib")
using namespace std;

/* рпк1 - модель с равноправными узлами, представляющими собой пары потоков,
первый читает входной файл и вычисляет результаты, второй записывает выходной файл.
Файл результатов общий, каждый результат записывается отдельно
(каждое обращение к файлу записывает один результат).
*/
//Объявление функций
void* InputAndSolution(void* args);
void* Output(void* args);
bool isPrime(long long n);

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
        pthread_create(&inputAndSolutionThread, NULL, InputAndSolution, this);
        pthread_create(&outputThread, NULL, Output, this);
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
                //std::cout << num << " считан" << std::endl;
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
//Нужно ли передавать row?
//Если да, решения будут записывать не по порядку
//Если нет, берём row из solution и записываем по порядку
static void WriteFile(FILE* f, Solution solution)
{
    std::string str;
    str += std::to_string(solution.lineNumber);
    str += ". ";
    str += std::to_string(solution.number);
    str += " - ";
    if (solution.isPrime)
        str += "простое число";
    else
        str += "не простое число";
    char line[1024];
    strcpy(line, str.c_str());
    fwrite(line, sizeof(char), 1024, f);
}

void* InputAndSolution(void* args)
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
        node->queueMutex.lock();
        node->solutions.push(solution);
        node->queueMutex.unlock();
    } while (true);
}
void* Output(void* args)
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
            //std::cout << solution.lineNumber << ". " << solution.number << " - " << (solution.isPrime ? "простое" : "не простое") << std::endl;
            WriteFile(outputF, solution);
            outputMutex.unlock();
        }
    } while (true);
}

bool isPrime(long long n)
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

    list<Node*> nodes;

    for (size_t i = 0; i < nodesCount; i++)
    {
        Node* node = new Node;
        nodes.push_back(node);
        node->StartSolving();
    }
    for (size_t i = 0; i < nodesCount; i++)
    {
        Node* node = nodes.front();
        nodes.pop_front();
        node->WaitSolving();
        delete(node);
    }
}

int main() {
	setlocale(LC_ALL, "RU");
    PrimeNumberSolver(1);
	return 0;
}