#include "pch.h"
#include <iostream>
#include <fstream>
#include "MultiplicativePRNG.h"
#include "algorithm"

using namespace std;


const char* printBool(bool value)
{
	return (value) ? "true" : "false";
}


double calcKolmogorovDistribution(double y, int tau)
{
	double sum = 0.0;
	short sign;
	for (int i = 1; i <= tau; i++)
	{
		sign = (i & 1) ? 1 : -1;
		sum += sign * exp(-2 * i * i * y * y);
	}
	return 1.0 - 2.0 * sum;
}



double calcKolmogorovDistanceUniform(double* sequence, int num)
{
	double result = 0.0;
	for (int i = 0; i < num; i++)
	{
		result = max(result, abs((double)(i + 1) / num - sequence[i]));
	}
	return result;
}

bool checkKolmogorovTestUniformQuantile(double quantile, double* sequence, int num)
{
	return (sqrt(num) * calcKolmogorovDistanceUniform(sequence, num) < quantile);
}

bool checkKolmogorovTestUniformSignificance(double significance, double* sequence, int num)
{
	return checkKolmogorovTestUniformQuantile(1.0 / calcKolmogorovDistribution(1.0 - significance, 1000), sequence, num);
}


int* calcFrequenciesEmperic(double* sequence, int num, int cellNum, double leftBorder, double rightBorder)
{
	int* result = new int[cellNum];
	double step = (rightBorder - leftBorder) / (cellNum + 1);
	double curBorder = step;
	int resultIndex = 0;

	for (int i = 0; i < cellNum; i++)
	{
		result[i] = 0.0;
	}

	for (int i = 0; i < num; i++)
	{
		if (sequence[i] < curBorder)
		{
			result[resultIndex]++;
		}
		else if (resultIndex < cellNum - 1)
		{
			resultIndex++;
			curBorder += step;
		}
	}

	return result;
}

bool checkPearsonTestUniform(double quantile, double* sequence, int num, int cellNum)
{
	int* empericFreq = calcFrequenciesEmperic(sequence, num, cellNum, 0.0, 1.0);
	double hi = 0.0;
	double uniformFreq = (double) num / cellNum;

	for(int i = 0; i < cellNum; i++)
	{
		hi += pow(empericFreq[i] - uniformFreq, 2.0) / uniformFreq;
	}

	delete[] empericFreq;

	return (hi < quantile);
}


double* methodMacLarenMarsaglie(PRNG* firstPRNG, PRNG* secondPRNG, int offset, int num)
{
	int firstSeqSize = num + offset;
	int index;
	double* lookupTable = new double[offset];
	double* firstSequence = new double[firstSeqSize];
	double* secondSequence = new double[num];
	double* result = new double[num];

	for (int i = 0; i < firstSeqSize; i++)
	{
		firstSequence[i] = firstPRNG->next();
	}
	for (int i = 0; i < num; i++)
	{
		secondSequence[i] = secondPRNG->next();
	}

	for (int i = 0; i < offset; i++)
	{
		lookupTable[i] = firstSequence[i];
	}
	for (int i = 0; i < num; i++)
	{
		index = (int)(secondSequence[i] * offset);
		result[i] = lookupTable[index];
		lookupTable[index] = firstSequence[i + offset];
	}

	delete[] lookupTable;
	delete[] firstSequence;
	delete[] secondSequence;

	return result;
}

int main()
{
    const long long module = pow(2, 31);
	const int multiplier = 262147;
	const int offset = 256;
	const int num = 1000;
	const int cellNum = 20;
	const double seed = 262147;
	const double significance = 0.05;
	const double quantileKolmogorov = 1.36;
	const double quantilePearson = 30.14;

	FILE* outputFile;
	fopen_s(&outputFile, "output.txt", "w");

	double* seqMultiplicativePRNG = new double[num];
	double* seqMacLarenMarsagliePRNG;
	PRNG* firstPRNG = new MultiplicativePRNG(module, seed, multiplier);
	PRNG* secondPRNG = new MultiplicativePRNG(module, 131075, 131075);

	printf("Multiplicative\n");
	fprintf(outputFile, "Multiplicative\n");
	for (int i = 0; i < num; i++)
	{
		seqMultiplicativePRNG[i] = firstPRNG->next();
		fprintf(outputFile, "%f\n", seqMultiplicativePRNG[i]);
	}
	sort(seqMultiplicativePRNG, &seqMultiplicativePRNG[num]);
	printf("Kolmogorov test: %s\n", printBool(checkKolmogorovTestUniformQuantile(quantileKolmogorov, seqMultiplicativePRNG, num)));
	fprintf(outputFile, "Kolmogorov test: %s\n", printBool(checkKolmogorovTestUniformQuantile(quantileKolmogorov, seqMultiplicativePRNG, num)));
	printf("Pearson test: %s\n", printBool(checkPearsonTestUniform(quantilePearson, seqMultiplicativePRNG, num, cellNum)));
	fprintf(outputFile, "Pearson test: %s\n", printBool(checkPearsonTestUniform(quantilePearson, seqMultiplicativePRNG, num, cellNum)));


	printf("\nMcLaren-Marsaglie\n");
	fprintf(outputFile, "\nMcLaren-Marsaglie\n");
	firstPRNG->reset();
	seqMacLarenMarsagliePRNG = methodMacLarenMarsaglie(firstPRNG, secondPRNG, offset, num);
	for (int i = 0; i < num; i++)
	{
		fprintf(outputFile, "%f\n", seqMacLarenMarsagliePRNG[i]);
	}
	sort(seqMacLarenMarsagliePRNG, &seqMacLarenMarsagliePRNG[num]);
	printf("Kolmogorov test: %s\n", printBool(checkKolmogorovTestUniformQuantile(quantilePearson, seqMacLarenMarsagliePRNG, num)));
	fprintf(outputFile, "Kolmogorov test: %s\n", printBool(checkKolmogorovTestUniformQuantile(quantilePearson, seqMacLarenMarsagliePRNG, num)));
	printf("Pearson test: %s\n", printBool(checkPearsonTestUniform(quantilePearson, seqMacLarenMarsagliePRNG, num, cellNum)));
	fprintf(outputFile, "Pearson test: %s\n", printBool(checkPearsonTestUniform(quantilePearson, seqMacLarenMarsagliePRNG, num, cellNum)));

	fclose(outputFile);

	delete firstPRNG;
	delete secondPRNG;
	delete[] seqMultiplicativePRNG;
	delete[] seqMacLarenMarsagliePRNG;
}
