# Variables to control Makefile operation
OVSPATH = OVS/
IOPATH = IO/
MITPATH = PartitionSort/
TRACEPATH = ClassBenchTraceGenerator/
TREEPATH = Trees/
UTILPATH = Utilities/

VPATH = $(OVSPATH) $(MITPATH) $(TRACEPATH) $(IOPATH) $(UTILPATH) $(TREEPATH) $(SPPATH)

CXX = g++
CXXFLAGS = -g -std=c++14 -fpermissive -O3 $(INCLUDE)

# Determine the operating system
ifeq ($(OS),Windows_NT)
	RM = del /Q
	FIXPATH = $(subst /,\,$1)
	EXECUTABLE = main.exe
else
	RM = rm -f
	FIXPATH = $1
	EXECUTABLE = main
endif

# Targets needed to bring the executable up to date
main: main.o Simulation.o InputReader.o OutputWriter.o trace_tools.o SortableRulesetPartitioner.o misc.o OptimizedMITree.o PartitionSort.o red_black_tree.o stack.o cmap.o TupleSpaceSearch.o HyperCuts.o HyperSplit.o TreeUtils.o IntervalUtilities.o MapExtensions.o
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $^

# -------------------------------------------------------------------

main.o: main.cpp ElementaryClasses.h SortableRulesetPartitioner.h InputReader.h Simulation.h cmap.h TupleSpaceSearch.h trace_tools.h PartitionSort.h IntervalUtilities.h hash.h OptimizedMITree.h
	$(CXX) $(CXXFLAGS) -c $<

Simulation.o: Simulation.cpp Simulation.h ElementaryClasses.h
	$(CXX) $(CXXFLAGS) -c $<

# Add other object file rules here...

# ** Utils **
IntervalUtilities.o: IntervalUtilities.cpp IntervalUtilities.h ElementaryClasses.h
	$(CXX) $(CXXFLAGS) -c $(UTILPATH)IntervalUtilities.cpp

MapExtensions.o : MapExtensions.cpp MapExtensions.h
	$(CXX) $(CXXFLAGS) -c $(UTILPATH)MapExtensions.cpp

clean:
	$(RM) $(call FIXPATH,*.o) $(EXECUTABLE)
