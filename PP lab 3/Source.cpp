#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
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

void* InputAndSolution(void* args);
void* Output(void* args);
bool isPrime(long long n);

FILE* OpenFileR()
{
    FILE* f = fopen("input.txt", "r");
    if (f == 0)
    {
        throw new exception("Файла input.txt нет!");
    }
    return f;
}
FILE* OpenFileW()
{
    FILE* f = fopen("output.txt", "w");
    if (f == 0)
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
    static std::mutex inputMutex;
    static int inputRowCount;
    static int maxRowCount;
    static FILE* inputF;

    static std::mutex outputMutex;
    static int outputRowCount;
    static FILE* outputF;

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
        pthread_create(&inputAndSolutionThread, NULL, InputAndSolution, this);
        pthread_create(&inputAndSolutionThread, NULL, Output, this);
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
                return _atoi64(buffer);
            }
            catch (std::invalid_argument e)
            {
                return -1;
            }
            break;
        }
        i++;
    }
}
//Нужно ли передавать row?
//Если да, решения будут записывать не по порядку
//Если нет, берём row из solution и записываем по порядку
static void WriteFile(FILE* f, int row, Solution solution)
{

}

void* InputAndSolution(void* args)
{
    Node* node = (Node*)args;
    do
    {
        node->inputMutex.lock();
        if (node->inputRowCount >= node->maxRowCount)
        {
            node->inputMutex.unlock();
            node->isFinishedMutex.lock();
            node->isFinished = true;
            node->isFinishedMutex.unlock();
            pthread_exit(0);
        }
        int row = node->inputRowCount;
        int num = ReadFile(node->inputF, row);
        node->inputRowCount++;
        node->inputMutex.unlock();
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
        if (node->solutions.empty())
        {
            node->isFinishedMutex.lock();
            bool isFinished = node->isFinished;
            node->isFinishedMutex.unlock();
            node->queueMutex.unlock();
            if (isFinished)
                pthread_exit(0);
        }
        else
        {
            Solution solution = node->solutions.front();
            node->solutions.pop();
            node->queueMutex.unlock();
            node->outputMutex.lock();
            WriteFile(node->outputF, node->outputRowCount, solution); //Или WriteFile(outputF, solution);
            node->outputRowCount++;
            node->outputMutex.unlock();
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

    Node::inputF = OpenFileR();
    Node::outputF = OpenFileW();

    Node::inputRowCount = 0;
    Node::maxRowCount = GetNumberOfLines(Node::inputF);
    Node::outputRowCount = 0;

    //list<Node> nodes;

    for (size_t i = 0; i < nodesCount; i++)
    {
        Node node;
        //nodes.push_back(node);
        node.StartSolving();
    }
}

int main() {
	setlocale(LC_ALL, "RU");
	return 0;
}