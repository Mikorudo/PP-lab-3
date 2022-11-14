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

/* ���1 - ������ � ������������� ������, ��������������� ����� ���� �������,
������ ������ ������� ���� � ��������� ����������, ������ ���������� �������� ����.
���� ����������� �����, ������ ��������� ������������ ��������
(������ ��������� � ����� ���������� ���� ���������).
*/
//���������� �������
void* InputAndSolution(void* args);
void* Output(void* args);
bool isPrime(long long n);

//���������� ���������� ����������
std::mutex inputMutex;
int inputRowCount;
int maxRowCount;
FILE* inputF;

std::mutex outputMutex;
int outputRowCount;
FILE* outputF;

//�������
FILE* OpenFileR()
{
    FILE* f = fopen("input.txt", "r");
    if (f == 0)
    {
        throw new exception("����� input.txt ���!");
    }
    return f;
}
FILE* OpenFileW()
{
    FILE* f = fopen("output.txt", "w");
    if (f == 0)
    {
        throw new exception("����� output.txt ���!");
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
//����� �� ���������� row?
//���� ��, ������� ����� ���������� �� �� �������
//���� ���, ���� row �� solution � ���������� �� �������
static void WriteFile(FILE* f, int row, Solution solution)
{

}

void* InputAndSolution(void* args)
{
    Node* node = (Node*)args;
    do
    {
        inputMutex.lock();
        if (inputRowCount >= maxRowCount)
        {
            inputMutex.unlock();
            node->isFinishedMutex.lock();
            node->isFinished = true;
            node->isFinishedMutex.unlock();
            pthread_exit(0);
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
            outputMutex.lock();
            WriteFile(outputF, outputRowCount, solution); //��� WriteFile(outputF, solution);
            outputRowCount++;
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
        throw new exception("���-�� ����� ������ 1");

    inputF = OpenFileR();
    outputF = OpenFileW();

    inputRowCount = 0;
    maxRowCount = GetNumberOfLines(inputF);
    outputRowCount = 0;

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