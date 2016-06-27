#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <cmath>

#include "alignments.hpp"
#include "../data/data.hpp"

Reads::Reads(std::string reference, std::string uLongRead, std::string cLongRead)
/* Constructor for general reads class - is the parent of GenericAlignments and ProovreadAlignments */
{
	ref = reference;
	ulr = uLongRead;
	clr = cLongRead;
	createMatrix();
}

Reads::Reads(const Reads &reads)
/* Copy constructor */
{
	// First, copy all member fields
	clr = reads.clr;
	ulr = reads.ulr;
	ref = reads.ref;
	rows = 0;
	columns = 0;
	matrix = NULL;
}

Reads::~Reads()
/* Delete the matrix when calling the destructor */
{
	deleteMatrix();
}

void Reads::reset(std::string reference, std::string uLongRead, std::string cLongRead)
/* Resets variables to new values and deletes and recreates matrix given new information */
{
	// Resets in case of need to reassign values of object
	ref = reference;
	ulr = uLongRead;
	clr = cLongRead;
	deleteMatrix();
	createMatrix();
}

std::string Reads::getClr()
/* Returns optimal cLR alignment ready to be written in 3-way MAF file */
{
	return clr;
}


std::string Reads::getUlr()
/* Returns optimal uLR alignment ready to be written in 3-way MAF file */
{
	return ulr;
}

std::string Reads::getRef()
/* Returns optimal ref alignment ready to be written in 3-way MAF file */
{
	return ref;
}

void Reads::createMatrix()
/* Preconstruct the matrix */
{
	std::string cleanedClr = clr; 
	cleanedClr.erase(std::remove(cleanedClr.begin(), cleanedClr.end(), ' '), cleanedClr.end());

	rows = cleanedClr.length() + 1;
	columns = ulr.length() + 1;
 
	try {
		matrix = new int*[rows];
	} catch( std::bad_alloc& ba ) {
		std::cerr << "Memory allocation failed; unable to create DP matrix.\n";
	}
	
	for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
		try {
			matrix[rowIndex] = new int[columns];
		} catch( std::bad_alloc& ba ) {
			std::cerr << "Memory allocation failed; unable to create DP matrix.\n";
		}
	}
}

void Reads::deleteMatrix()
/* Delete the matrix allocated in the heap */
{
	for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
		if (matrix[rowIndex] != NULL) {
			delete matrix[rowIndex];
		} 	
	}

	if (matrix != NULL) {
		delete matrix;
	}
}

int Reads::cost(char refBase, char cBase)
/* Cost function for dynamic programming algorithm */
{
	if ( islower(cBase) ) {
		return 0;
	} else if ( toupper(refBase) == cBase ) {
		return 0;
	} else {
		// Ideally, in an alignment between cLR and ref we want to minimize the number of discrepancies
		// as much as possible, so if both bases are different, we assign a cost of 2.
		return 2;
	}
}

/*--------------------------------------------------------------------------------------------------------*/

GenericAlignments::GenericAlignments(std::string reference, std::string uLongRead, std::string cLongRead)
	: Reads(reference, uLongRead, cLongRead)
/* Constructor */
{ initialize(); }

void GenericAlignments::reset(std::string reference, std::string uLongRead, std::string cLongRead)
/* Resets variables to new values and deletes and recreates matrix given new information */
{ 
	Reads::reset(reference, uLongRead, cLongRead);
	initialize(); 
}

void GenericAlignments::initialize()
/* Given cLR, uLR and ref sequences, construct the DP matrix for the optimal alignments. 
 * Requires these member variables to be set before use. */
{
	int cIndex;
	int urIndex;
	bool isEndingLC;
	int keep;
	int substitute;
	int insert;
	int deletion;
	int infinity = std::numeric_limits<int>::max();

	// Set the base cases for the DP matrix
	for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
		matrix[rowIndex][0] = rowIndex;	
	}
	for (int columnIndex = 1; columnIndex < columns; columnIndex++) {
		matrix[0][columnIndex] = columnIndex;
	}

	// Find the optimal edit distance such that all uncorrected segments of clr are aligned with uncorrected
 	// portions of ulr. 
	for (int rowIndex = 1; rowIndex < rows; rowIndex++) {
		for (int columnIndex = 1; columnIndex < columns; columnIndex++) {
			cIndex = rowIndex - 1;
			urIndex = columnIndex -  1;

			// Determine if the current letter in clr is lowercase and is followed by an upper case letter
			// i.e. if the current letter in clr is an ending lowercase letter
			if ( cIndex < clr.length() - 1 && islower(clr[cIndex]) && isupper(clr[cIndex+1]) ) {
				isEndingLC = true;	
			} else {
				isEndingLC = false;
			}

			if (isEndingLC) {
				// If both letters are the same, we can either keep both letters or deletion the one from
				// clr. If they are different, we can't keep both so we can only consider deleting the
				// one from clr.
				if ( toupper(ulr[urIndex]) == toupper(clr[cIndex]) ) {
					keep = std::abs( matrix[rowIndex-1][columnIndex-1] + cost(ref[urIndex], clr[cIndex]) );
					deletion = std::abs( matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-') );
					matrix[rowIndex][columnIndex] = std::min(keep, deletion); 
				} else {
					matrix[rowIndex][columnIndex] = std::abs( matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-') ); 
				}
			} else if (islower(clr[cIndex])) {
				if ( toupper( ulr[urIndex] ) == toupper( clr[cIndex] ) ) {
					// Keep the characters if they are the same
					matrix[rowIndex][columnIndex] = std::abs( matrix[rowIndex-1][columnIndex-1] + cost(ref[urIndex], clr[cIndex]) );
				} else if (ulr[urIndex] == '-') {
					// If uLR has a dash, the optimal solution is just to call a deletion.
					matrix[rowIndex][columnIndex] = matrix[rowIndex][columnIndex-1]; //Zero cost deletion
				} else {
					// Setting the position in the matrix to infinity ensures that we can never
					// find an alignment where the uncorrected segments are not perfectly aligned.
					matrix[rowIndex][columnIndex] = infinity;
				}
			} else {
				// Usual Levenshtein distance equations.
				deletion = std::abs( matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-') );
				insert = std::abs( matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]) );
				substitute = std::abs( matrix[rowIndex-1][columnIndex-1] + cost(ref[urIndex], clr[cIndex]) );
				matrix[rowIndex][columnIndex] = std::min( deletion, std::min(insert, substitute) ); 
			}
		}		
	}
	findAlignments();
}

void GenericAlignments::findAlignments()
/* Backtracks through the DP matrix to find the optimal alignments. 
 * Follows same schema as the DP algorithm. */
{
	std::string clrMaf = "";
	std::string ulrMaf = "";
	std::string refMaf = "";
	int rowIndex = rows - 1;
	int columnIndex = columns - 1;
	int cIndex;
	int urIndex;
	int insert;
	int deletion;
	int substitute;
	int currentCost;
	bool isEndingLC;
	int infinity = std::numeric_limits<int>::max();

	// Follow the best path from the bottom right to the top left of the matrix.
	// This is equivalent to the optimal alignment between ulr and clr.
	// The path we follow is restricted to the conditions set when computing the matrix,
	// i.e. we can never follow a path that the edit distance equations do not allow.
	while (rowIndex > 0 || columnIndex > 0) {
		/*
		std::cout << "rowIndex == " << rowIndex << "\n";
		std::cout << "columnIndex == " << columnIndex << "\n";
		std::cout << "Before\n";
		std::cout << "clrMaf == " << clrMaf << "\n";
		std::cout << "ulrMaf == " << ulrMaf << "\n";
		std::cout << "refMaf == " << refMaf << "\n";
		*/

		urIndex = columnIndex - 1;
		cIndex = rowIndex - 1;
		currentCost = matrix[rowIndex][columnIndex];

		// Set the costs of the different operations, 
		// ensuring we don't go out of bounds of the matrix.
		if (rowIndex > 0 && columnIndex > 0) {
			deletion = matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-');
			insert = matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]);
			substitute = matrix[rowIndex-1][columnIndex-1] + cost(ref[urIndex], clr[cIndex]);	
		} else if (rowIndex <= 0 && columnIndex > 0) {
			deletion = matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-');
			insert = infinity;
			substitute = infinity;
		} else if (rowIndex > 0 && columnIndex <= 0) {
			deletion = infinity;
			insert = matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]);
			substitute = infinity;
		} 

		// Make sure we follow the same path as dictated by the edit distance equations. 
		if ( (cIndex < clr.length() - 1 && islower(clr[cIndex]) && isupper(clr[cIndex+1])) || 
			(cIndex == clr.length() - 1 && islower(clr[cIndex])) ) {
			isEndingLC = true;	
		} else {
			isEndingLC = false;
		}
		if (rowIndex == 0 || columnIndex == 0) {
				//std::cout << "Path 6\n";
				if (rowIndex == 0) {
					//std::cout << "Deletion\n";
					clrMaf = '-' + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					columnIndex--;
				} else {
					//std::cout << "Insertion\n";
					clrMaf = clr[cIndex] + clrMaf;
					ulrMaf = '-' + ulrMaf;
					refMaf = '-' + refMaf;
					rowIndex--;
				}
		} else if (isEndingLC) {
			if ( toupper( ulr[urIndex] ) == toupper( clr[cIndex] ) ) {
				//std::cout << "Path 1\n";
				if (deletion == currentCost) {
					//std::cout << "Deletion\n";
					clrMaf = '-' + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					columnIndex--;
				} else if (substitute == currentCost) {
					//std::cout << "Substitution\n";
					clrMaf = clr[cIndex] + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					rowIndex--;
					columnIndex--;
				} else {
					std::cerr << "ERROR CODE 1: No paths found. Terminating backtracking.\n";	
					rowIndex = 0;
					columnIndex = 0;
				}
			} else {
				//std::cout << "Path 2\n";
				if (deletion == currentCost) {
					//std::cout << "Deletion\n";
					clrMaf = '-' + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					columnIndex--;
				} else {
					std::cerr << "ERROR CODE 2: No paths found. Terminating backtracking.\n";
					rowIndex = 0;
					columnIndex = 0;
				}
			}
		} else if (islower(clr[cIndex]) && rowIndex > 0 && columnIndex > 0) {
			if ( toupper( ulr[urIndex] ) == toupper( clr[cIndex] ) ) {
				//std::cout << "Path 3\n";
				if (substitute == currentCost) {
					//std::cout << "Substitution\n";
					clrMaf = clr[cIndex] + clrMaf;	
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					rowIndex--;
					columnIndex--;
				} else {
					std::cerr << "ERROR CODE 3: No paths found. Terminating backtracking.\n";
					rowIndex = 0;
					columnIndex = 0;
				}
			} else if (ulr[urIndex] == '-') {
				//std::cout << "Path 4\n";
				deletion = matrix[rowIndex][columnIndex-1];
				if (deletion == currentCost) {
					//std::cout << "Deletion\n";
					clrMaf = '-' + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					columnIndex--;
				} else {
					std::cerr << "ERROR CODE 4: No paths found. Terminating backtracking.\n";
					rowIndex = 0;
					columnIndex = 0;
				}
			} else {
				std::cerr << "ERROR CODE 5: No paths found. Terminating backtracking.\n";
				rowIndex = 0;
				columnIndex = 0;
			}
		} else {
			//std::cout << "Path 5\n";
			if (deletion == currentCost) {
				//std::cout << "Deletion\n";
				clrMaf = '-' + clrMaf;
				ulrMaf = ulr[urIndex] + ulrMaf;
				refMaf = ref[urIndex] + refMaf;
				columnIndex--;
			} else if (insert == currentCost) {
				//std::cout << "Insertion\n";
				clrMaf = clr[cIndex] + clrMaf;
				ulrMaf = '-' + ulrMaf;
				refMaf = '-' + refMaf;
				rowIndex--;
			} else if (substitute == currentCost) {
				//std::cout << "Substitution\n";
				clrMaf = clr[cIndex] + clrMaf;
				ulrMaf = ulr[urIndex] + ulrMaf;
				refMaf = ref[urIndex] + refMaf;
				rowIndex--;
				columnIndex--;
			} else {
				std::cerr << "ERROR CODE 6: No paths found. Terminating backtracking.\n";
				rowIndex = 0;
				columnIndex = 0;
			}
		} 		
		/*
		std::cout << "After\n";
		std::cout << "clrMaf == " << clrMaf << "\n";
		std::cout << "ulrMaf == " << ulrMaf << "\n";
		std::cout << "refMaf == " << refMaf << "\n\n";
		*/
	}

	clr = clrMaf;
	ulr = ulrMaf;
	ref = refMaf;
}

/* --------------------------------------------------------------------------------------------- */

ProovreadAlignments::ProovreadAlignments(std::string reference, std::string uLongRead, std::string cLongRead)
	: Reads(reference, uLongRead, cLongRead)
/* Constructor - is a child class of Reads */
{ initialize(); }

void ProovreadAlignments::reset(std::string reference, std::string uLongRead, std::string cLongRead)
/* Resets the alignment object to be used again */
{
	Reads::reset(reference, uLongRead, cLongRead);
 	initialize(); 
}

void ProovreadAlignments::initialize()
/* Create the DP matrix similar to generic alignment object */
{
	// Split the clr into its corrected parts
	std::vector< std::string > trimmedClrs = split(clr);
	// Remove spaces in clr
	clr.erase(std::remove(clr.begin(), clr.end(), ' '), clr.end());

	rows = clr.length() + 1;
	columns = ref.length() + 1;

	int cIndex;
	int urIndex;
	int substitute;
	int insert;
	int deletion;

	int lastBaseIndex = -1;
	bool isLastBase;
	
	// Record the indices of the last bases of all the reads
	for (int index = 0; index < trimmedClrs.size(); index++) {
		lastBaseIndex = lastBaseIndex + trimmedClrs.at(index).length();
		lastBaseIndices.push_back(lastBaseIndex);	
	}

	// Set the base cases for the DP matrix
	for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
		matrix[rowIndex][0] = rowIndex;	
	}
	for (int columnIndex = 1; columnIndex < columns; columnIndex++) {
		matrix[0][columnIndex] = 0;
	}

	// Find the minimal edit distances
	for (int rowIndex = 1; rowIndex < rows; rowIndex++) {
		for (int columnIndex = 1; columnIndex < columns; columnIndex++) {
			cIndex = rowIndex - 1;
			urIndex = columnIndex - 1;

			// Check if cIndex is the last base of a read
			if (std::find( lastBaseIndices.begin(), lastBaseIndices.end(), cIndex ) != lastBaseIndices.end()) {
				isLastBase = true;
			} else {
				isLastBase = false;
			} 

			if (isLastBase) {
				deletion = matrix[rowIndex][columnIndex-1];
			} else {
				deletion = matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-');
			}	

			insert = matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]);
			substitute = matrix[rowIndex-1][columnIndex-1] + cost(clr[cIndex], ref[urIndex]);
			matrix[rowIndex][columnIndex] = std::min( deletion, std::min( insert, substitute ) );
		}
	}

	findAlignments();
}

void ProovreadAlignments::findAlignments()
/* Construct the optimal alignments between the three reads */
{
	std::string clrMaf = "";
	std::string ulrMaf = "";
	std::string refMaf = "";
	int rowIndex = rows - 1;
	int columnIndex = columns - 1;
	int cIndex;
	int urIndex;
	int insert;
	int deletion;
	int substitute;
	int currentCost;
	int infinity = std::numeric_limits<int>::max();
	bool isLastBase;

	// Follow the best path from the bottom right to the top left of the matrix.
	// This is equivalent to the optimal alignment between ulr and clr.
	// The path we follow is restricted to the conditions set when computing the matrix,
	// i.e. we can never follow a path that the edit distance equations do not allow.
	while (rowIndex > 0 || columnIndex > 0) {
		/*
		std::cout << "rowIndex == " << rowIndex << "\n";
		std::cout << "columnIndex == " << columnIndex << "\n";
		std::cout << "Before\n";
		std::cout << "clrMaf == " << clrMaf << "\n";
		std::cout << "ulrMaf == " << ulrMaf << "\n";
		std::cout << "refMaf == " << refMaf << "\n";
		*/

		urIndex = columnIndex - 1;
		cIndex = rowIndex - 1;
		currentCost = matrix[rowIndex][columnIndex];

		// Check if cIndex is the last base of a read
		if (std::find(lastBaseIndices.begin(), lastBaseIndices.end(), cIndex) != lastBaseIndices.end()) {
			isLastBase = true;
		} else {
			isLastBase = false;
		} 

		// Set the costs of the different operations, 
		// ensuring we don't go out of bounds of the matrix.
		if (rowIndex > 0 && columnIndex > 0) {
			if (isLastBase) {
				deletion = matrix[rowIndex][columnIndex-1]; 
			} else {
				deletion = matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-');
			}
			insert = matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]);
			substitute = matrix[rowIndex-1][columnIndex-1] + cost(ref[urIndex], clr[cIndex]);	
		} else if (rowIndex <= 0 && columnIndex > 0) {
			if (isLastBase) {
				deletion = matrix[rowIndex][columnIndex-1];
			} else {
				deletion = matrix[rowIndex][columnIndex-1] + cost(ref[urIndex], '-');
			}
			insert = infinity;
			substitute = infinity;
		} else if (rowIndex > 0 && columnIndex <= 0) {
			deletion = infinity;
			insert = matrix[rowIndex-1][columnIndex] + cost('-', clr[cIndex]);
			substitute = infinity;
		} 

		if (rowIndex == 0 || columnIndex == 0) {
				//std::cout << "Path 6\n";
				if (rowIndex == 0) {
					//std::cout << "Deletion\n";
					clrMaf = '-' + clrMaf;
					ulrMaf = ulr[urIndex] + ulrMaf;
					refMaf = ref[urIndex] + refMaf;
					columnIndex--;
				} else {
					//std::cout << "Insertion\n";
					clrMaf = clr[cIndex] + clrMaf;
					ulrMaf = '-' + ulrMaf;
					refMaf = '-' + refMaf;
					rowIndex--;
				}
		} else if (deletion == currentCost) {
			//std::cout << "Deletion\n";
			clrMaf = '-' + clrMaf;
			ulrMaf = ulr[urIndex] + ulrMaf;
			refMaf = ref[urIndex] + refMaf;
			columnIndex--;
		} else if (insert == currentCost) {
			//std::cout << "Insertion\n";
			clrMaf = clr[cIndex] + clrMaf;
			ulrMaf = '-' + ulrMaf;
			refMaf = '-' + refMaf;
			rowIndex--;
		} else if (substitute == currentCost) {
			//std::cout << "Substitution\n";
			clrMaf = clr[cIndex] + clrMaf;
			ulrMaf = ulr[urIndex] + ulrMaf;
			refMaf = ref[urIndex] + refMaf;
			rowIndex--;
			columnIndex--;
		} else {
			std::cerr << "ERROR CODE 6: No paths found. Terminating backtracking.\n";
			rowIndex = 0;
			columnIndex = 0;
		}
	}

	clr = clrMaf;
	ulr = ulrMaf;
	ref = refMaf;
}