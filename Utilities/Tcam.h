#ifndef TCAM_H
#define TCAM_H

#include <vector>

#include "../ElementaryClasses.h"

namespace tcam {
unsigned int SizeAsPrefixes(const std::array<unsigned int, 2>& range);
unsigned int NumOfPrefixRules(const Rule& r);
unsigned int SizeAsPrefixRules(const std::vector<Rule>& rules);
}  // namespace tcam
#endif
