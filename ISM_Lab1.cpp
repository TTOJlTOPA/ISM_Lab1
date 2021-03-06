#include "pch.h"
#include <iostream>
#include <fstream>
#include "MultiplicativePRNG.h"
#include "algorithm"

#define GNUPLOT             "gnuplot"
#define GNUPLOT_WIN_WIDTH   1280
#define GNUPLOT_WIN_HEIGHT  720

using namespace std;

FILE* outputFile;

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
	double distance = sqrt(num) * calcKolmogorovDistanceUniform(sequence, num);
	fprintf(outputFile, "Distance = %f\n", distance);
	return (distance < quantile);
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

	fprintf(outputFile, "Hi = %f\n", hi);

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
	FILE* pipeMul;
	FILE* pipeMC;

#ifdef WIN32
    pipeMul = _popen(GNUPLOT, "w");
	pipeMC = _popen(GNUPLOT, "w");
#else
    pipeMul = popen(GNUPLOT, "w");
	pipeMC = popen(GNUPLOT, "w");
#endif

    const long long module = pow(2, 31);
	const int multiplier = 262147;
	const int offset = 256;
	const int num = 1000;
	const int cellNum = 20;
	const double seed = 262147;
	const double significance = 0.05;
	const double quantileKolmogorov = 1.36;
	const double quantilePearson = 30.14;

	bool testResult;
	double* seqMultiplicativePRNG = new double[num];
	double* seqMacLarenMarsagliePRNG;
	PRNG* firstPRNG = new MultiplicativePRNG(module, seed, multiplier);
	PRNG* secondPRNG = new MultiplicativePRNG(module, 131075, 131075);

	fopen_s(&outputFile, "output.txt", "w");

	printf("Multiplicative\n");
	fprintf(outputFile, "Multiplicative\n");
	for (int i = 0; i < num; i++)
	{
		seqMultiplicativePRNG[i] = firstPRNG->next();
		fprintf(outputFile, "%f\n", seqMultiplicativePRNG[i]);
	}
	sort(seqMultiplicativePRNG, &seqMultiplicativePRNG[num]);
	testResult = checkKolmogorovTestUniformQuantile(quantileKolmogorov, seqMultiplicativePRNG, num);
	printf("Kolmogorov test: %s\n", printBool(testResult));
	fprintf(outputFile, "Kolmogorov test: %s\n", printBool(testResult));
	testResult = checkPearsonTestUniform(quantilePearson, seqMultiplicativePRNG, num, cellNum);

	printf("Pearson test: %s\n", printBool(testResult));
	fprintf(outputFile, "Pearson test: %s\n", printBool(testResult));


	printf("\nMacLaren-Marsaglie\n");
	fprintf(outputFile, "\nMacLaren-Marsaglie\n");
	firstPRNG->reset();
	seqMacLarenMarsagliePRNG = methodMacLarenMarsaglie(firstPRNG, secondPRNG, offset, num);
	for (int i = 0; i < num; i++)
	{
		fprintf(outputFile, "%f\n", seqMacLarenMarsagliePRNG[i]);
	}
	sort(seqMacLarenMarsagliePRNG, &seqMacLarenMarsagliePRNG[num]);
	testResult = checkKolmogorovTestUniformQuantile(quantilePearson, seqMacLarenMarsagliePRNG, num);
	printf("Kolmogorov test: %s\n", printBool(testResult));
	fprintf(outputFile, "Kolmogorov test: %s\n", printBool(testResult));
	testResult = checkPearsonTestUniform(quantilePearson, seqMacLarenMarsagliePRNG, num, cellNum);
	printf("Pearson test: %s\n", printBool(testResult));
	fprintf(outputFile, "Pearson test: %s\n", printBool(testResult));

	fclose(outputFile);


	if (pipeMul != nullptr && pipeMC != nullptr)
    {
		fprintf(pipeMul, "n=%d\n", cellNum);
		fprintf(pipeMul, "min=%f\n", 0.0);
		fprintf(pipeMul, "max=%f\n", 1.0);
		fprintf(pipeMul, "width=(max-min)/n\n");
		fprintf(pipeMul, "hist(x,width)=width*floor(x/width)+width/2.0\n");
		fprintf(pipeMul, "set xrange [min:max]\n");
		fprintf(pipeMul, "set yrange [0:]\n");
		fprintf(pipeMul, "set offset graph 0.05,0.05,0.05,0.0\n");
		fprintf(pipeMul, "set xtics min,(max-min)/5,max\n");
		fprintf(pipeMul, "set boxwidth width*0.9\n");
		fprintf(pipeMul, "set style fill solid 0.5\n");
		fprintf(pipeMul, "set tics out nomirror\n");

        fprintf(pipeMul, "set term wxt size %d, %d\n", GNUPLOT_WIN_WIDTH, GNUPLOT_WIN_HEIGHT);
        fprintf(pipeMul, "set title \"Multiplicative (n=%d)\"\n", num);
        fprintf(pipeMul, "set xlabel \"Values\"\n");
        fprintf(pipeMul, "set ylabel \"Frequency\"\n");
        fprintf(pipeMul, "plot '-' u (hist($1,width)):(1.0) smooth freq w boxes lc rgb\"green\" notitle\n");

		for (int i = 0; i < num; i++)
		{
			fprintf(pipeMul, "%lf\n", seqMultiplicativePRNG[i]);
		}

		fprintf(pipeMul, "%s\n", "e");
        fflush(pipeMul);


		fprintf(pipeMC, "n=%d\n", cellNum);
		fprintf(pipeMC, "min=%f\n", 0.0);
		fprintf(pipeMC, "max=%f\n", 1.0);
		fprintf(pipeMC, "width=(max-min)/n\n");
		fprintf(pipeMC, "hist(x,width)=width*floor(x/width)+width/2.0\n");
		fprintf(pipeMC, "set xrange [min:max]\n");
		fprintf(pipeMC, "set yrange [0:]\n");
		fprintf(pipeMC, "set offset graph 0.05,0.05,0.05,0.0\n");
		fprintf(pipeMC, "set xtics min,(max-min)/5,max\n");
		fprintf(pipeMC, "set boxwidth width*0.9\n");
		fprintf(pipeMC, "set style fill solid 0.5\n");
		fprintf(pipeMC, "set tics out nomirror\n");

        fprintf(pipeMC, "set term wxt size %d, %d\n", GNUPLOT_WIN_WIDTH, GNUPLOT_WIN_HEIGHT);
        fprintf(pipeMC, "set title \"MacLaren-Marsaglie (n=%d)\"\n", num);
        fprintf(pipeMC, "set xlabel \"Values\"\n");
        fprintf(pipeMC, "set ylabel \"Frequency\"\n");
        fprintf(pipeMC, "plot '-' u (hist($1,width)):(1.0) smooth freq w boxes lc rgb\"green\" notitle\n");

		for (int i = 0; i < num; i++)
		{
			fprintf(pipeMC, "%lf\n", seqMacLarenMarsagliePRNG[i]);
		}

		fprintf(pipeMC, "%s\n", "e");
        fflush(pipeMC);

		cin.clear();
        cin.get();

#ifdef WIN32
        _pclose(pipeMul);
        _pclose(pipeMC);
#else
        pclose(pipeMul);
        pclose(pipeMC);
#endif
	}


	delete firstPRNG;
	delete secondPRNG;
	delete[] seqMultiplicativePRNG;
	delete[] seqMacLarenMarsagliePRNG;
}
