#ifndef MEASURES_H
#define MEASURES_H

// The following three structs are simple containers for the proportion of the respective
// mutations between a corrected long read segment and its respective uncorrected long
// read segment
struct InsertionProportion
{
        int64_t cRead;
        int64_t uRead;
};

struct DeletionProportion
{
        int64_t cRead;
        int64_t uRead;
};

struct SubstitutionProportion
{
        int64_t cRead;
        int64_t uRead;
};

struct CorrespondingSegments
/* This struct is a simple container for corrected segments of corrected long reads and its
 *  * respective segments in the uncorrected long read and reference sequences. */
{
        std::string cReadSegment;
        std::string uReadSegment;
        std::string refSegment;
};

std::vector< CorrespondingSegments > getCorrespondingSegmentsList(std::string cRead, std::string uRead, std::string ref);
/* Returns a vector of all the CorrespondingSegments of the given cLR, uLR and reference sequences. */

SubstitutionProportion getSubstitutionProportion( CorrespondingSegments correspondingSegments);
/* Returns the proportion of substitutions between the reads in the correspondingSegments */

InsertionProportion getInsertionProportion( CorrespondingSegments correspondingSegments);
/* Returns the proportion of insertions between the reads in the correspondingSegments */

DeletionProportion getDeletionProportion( CorrespondingSegments correspondingSegments);
/* Returns the proportion of deletions between the reads in the correspondingSegments */

int64_t getSubstitutions(std::string ref, std::string read);
// Returns the number of substitutions between the reference and read string

int64_t getInsertions(std::string ref, std::string read);
// Returns the number of insertions between the reference and read string

int64_t getDeletions(std::string ref, std::string read);
// Returns the number of insertions between the reference and read string

#endif // MEASURES_H
