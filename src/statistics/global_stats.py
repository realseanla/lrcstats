from __future__ import division
import data
import sys
import getopt

def collectStatistics(trimmedData):
	'''
	Collect statistics from data.
	Inputs
	- (list of TrimmedDatum objects) trimmedData: contains the data
	Outputs
	- (dict of ints) statistics: contains the statistics
	'''
	statistics = {correctedBases_k : 0,
			correctedDeletions_k : 0,
			correctedInsertions_k : 0,
			correctedSubstitutions_k : 0,
			uncorrectedBases_k : 0,
			uncorrectedDeletions_k : 0,
			uncorrectedInsertions_k : 0,
			uncorrectedSubstitutions_k : 0}	
			
	for datum in trimmedData:
		# Corrected read statistics
		statistics[correctedBases_k] += datum.getCorrLength()
		statistics[correctedDeletions_k] += datum.getCorrDel()
		statistics[correctedInsertions_k] += datum.getCorrIns()
		statistics[correctedSubstitutions_k] += datum.getCorrSub()
		# Uncorrected read statistics
		statistics[uncorrectedBases_k] += datum.getUncorrLength()
		statistics[uncorrectedDeletions_k] += datum.getUncorrDel()
		statistics[uncorrectedInsertions_k] += datum.getUncorrIns()
		statistics[uncorrectedSubstitutions_k] += datum.getUncorrSub()		

	return statistics

def writeStatisticsSummary(outputPath, statistics):
	'''
	Write the summary of the statistics into a text file.
	Inputs
	- (string) outputPath: the path to save file
	- (dict of ints) statistics: contains the statistics
	'''
	# Get the corrected read statistics
	correctedThroughput = statistics[correctedBases_k]
	correctedMutations = statistics[correctedDeletions_k] + statistics[correctedInsertions_k] \
				+ statistics[correctedSubstitutions_k]
	correctedTruePositives = correctedThroughput - correctedMutations
	correctedErrorRate = correctedMutations/correctedThroughput

	# Get the uncorrected read statistics
	uncorrectedThroughput = statistics[uncorrectedBases_k]
	uncorrectedMutations = statistics[uncorrectedDeletions_k] + statistics[uncorrectedInsertions_k] \
				+ statistics[uncorrectedSubstitutions_k]
	uncorrectedTruePositives = uncorrectedThroughput - uncorrectedMutations
	uncorrectedErrorRate = uncorrectedMutations/uncorrectedThroughput

	with open(outputPath, 'w') as file:
		# file.write("%s\n" % (name))
		header = "            Error Rate   Throughput   Correct   Incorrect   Deletions   Insertions   Substitutions\n"
		file.write(header)

		correctedLine = "Corrected   %f %d %d %d %d %d %d\n" \
				% (correctedErrorRate, correctedThroughput, correctedTruePositives, 
					correctedMutations, statistics[correctedDeletions_k], statistics[correctedInsertions_k], 
					statistics[correctedSubstitutions_k])	
		file.write(correctedLine)

		uncorrectedLine = "Uncorrected %f %d %d %d %d %d %d\n" \
				% (uncorrectedErrorRate, uncorrectedThroughput, uncorrectedTruePositives, 
					uncorrectedMutations, statistics[uncorrectedDeletions_k], 
					statistics[uncorrectedInsertions_k], statistics[uncorrectedSubstitutions_k])	
		file.write(uncorrectedLine)

# Global variables for data dict
correctedBases_k = "CORRECTED BASES"
correctedDeletions_k = "CORRECTED DELETIONS"
correctedInsertions_k = "CORRECTED INSERTIONS"
correctedSubstitutions_k = "CORRECTED SUBSTITUTIONS"

uncorrectedBases_k = "UNCORRECTED BASES"
uncorrectedDeletions_k = "UNCORRECTED DELETIONS"
uncorrectedInsertions_k = "UNCORRECTED INSERTIONS"
uncorrectedSubstitutions_k = "UNCORRECTED SUBSTITUTIONS"

helpMessage = "Summarize global long read correction data statistics."
usageMessage = "Usage: %s [-h help and usage] [-i stats file input path] [-o output path]" % (sys.argv[0])
options = "hi:o:t"

try:
	opts, args = getopt.getopt(sys.argv[1:], options)
except getopt.GetoptError:
	print "Error: unable to read command line arguments."
	sys.exit(2)

if len(sys.argv) == 1:
	print usageMessage
	sys.exit(2)

inputPath = None
outputPath = None
testRun = False

for opt, arg in opts:
	if opt == '-h':
		print helpMessage
		print usageMessage
		sys.exit()
	elif opt == '-i':
		inputPath = arg
	elif opt == '-o':
		outputPath = arg
	elif opt == '-t':
		testRun = True

optsIncomplete = False

if inputPath is None and not testRun:
	print "Please specify the input path."
	optsIncomplete = True
if outputPath is None:
	print "Please specify the output path."
	optsIncomplete = True

if optsIncomplete:
	print usageMessage
	sys.exit(2)

# We don't use the untrimmedData
trimmedData, untrimmedData = data.retrieveRawData(inputPath)

statistics = collectStatistics(trimmedData)
writeStatisticsSummary(outputPath, statistics)