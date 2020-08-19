// $Id: restoration.h 2009-11-09 15:00:00Z d3x593 $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _RESTORATION_H
#define _RESTORATION_H

#include "powerflow.h"
#include "powerflow_library.h"

typedef struct s_ChainNode {
	int data;
	struct s_ChainNode *link;
} CHAINNODE;	///Node of the linked list

typedef struct s_intVect {
	int *data;
	int currSize;
	int maxSize;
} INTVECT;

typedef struct s_ChordSet {
	int *data_1;	//Column 1
	int *data_2;	//Column 2 - semi-silly to do it this way, but I have reasons
	int currSize;
	int maxSize;
} CHORDSET;

typedef struct s_LocSet {
	int *data_1;	//Column 1
	int *data_2;	//Column 2 - semi-silly to do it this way, but I have reasons
	int *data_3;	//Column 3 - Expanded silly, but makes the ML transition easier
	int *data_4;	//Column 4 - Expanded silly, but makes the ML transition easier
	int currSize;
	int maxSize;
} LOCSET;

typedef struct s_CandSwOpe {
	int *data_1;	//Column 1														-- Switch to be opened (1)
	int *data_2;	//Column 2 - semi-silly to do it this way, but I have reasons	-- Switch to be opened (2)
	int *data_3;	//Column 3 - Expanded silly, but makes the ML transition easier	-- Switch to be closed (1)
	int *data_4;	//Column 4 - Expanded silly, but makes the ML transition easier	-- Switch to be closed (2)
	int *data_5;	//Column 5 - Expanded silly, but makes the ML transition easier -- previous cyclic interfhange operation
	double *data_6;	//Column 6 - Expanded silly, but makes the ML transition easier -- Amount of loads not able to be restored or overload.  Not sure
	int *data_7;	//Column 7 - Expanded silly, but makes the ML transition easier -- Feeder number overloaded?
	int currSize;
	int maxSize;
} CANDSWOP;

typedef struct s_BranchVertices {
	int from_vert;	//From vertex
	int to_vert;	//To vertex
} BRANCHVERTICES;

//ChainNode class
class Chain
{
public: //member properties
	CHAINNODE *first;												//first node of linked list
	CHAINNODE *last;												//last node of linked list
public: //member constructor
	inline Chain()
	{
		first = NULL;
		last = NULL;
	}
public: //member functions
	void delAllNodes(void);					//deletes all nodes in linked list
	bool isempty(void);						//checks if linked list is empty
	int getLength(void);					//returns the length of the linked list
	int search(int sData);					//search linkedlist for given data, return index
	bool modify(int oldData, int newData);	//replace oldData with newData and return 1.  If no oldData, return 0
	void addNode(int kIndex, int newData);	//add new node after kth node
	void deleteNode(int dData);				//delete 1st node found containing dData.  If not found, do nothing
	void copy(Chain *ilist);				//copy a linked list into a new linked list
	void append(int newData);				//add a new node containing newData at the end of the linked list
};

//LinkedQueue class
class LinkedQueue : public Chain
{
public:
	inline LinkedQueue():Chain(){};		//Constructor link
	void enQueue(int newData);			//add new node at end of the queue
	int deQueue(void);					//delete first node of queue and return its data
};

//ChainIterator class
class ChainIterator
{
public: // member properties
	CHAINNODE *location;			//a pointer to a node of a linked list
public: // class constructor
	inline ChainIterator()
	{
		location = NULL;
	}
public: // member functions
	int initialize(Chain *lList);	//initializes the iterator, takes as argument lList of class Chain
	int next(void);					//move the pointer to the next node
};

//LinkedBase class
class LinkedBase
{
public: // member properties
	int numVertices;			//number of vertices in graph
	int numEdges;				//number of edges in graph
	Chain **adjList;			//adjacency list - just 0's and 1's
	ChainIterator **iterPos;	//array of pointers to linked-list iterators
	int source;					//source vertex
	int *status_value;			//should be enumeration - 0:not discovered, 1:discovered but not finished, 2:finished
	int *parent_value;			//the parents of all vertices.  -1 means no parent
	double *dist;				//distance for the source to each vertext (For Breath-First-Search or BFS)
	TIMESTAMP dfs_time;			//Previous timestamp - mainly for intialization
	TIMESTAMP *dTime;			//timestamp array I for DFS, records when each vertex is discovered
	TIMESTAMP *fTime;			//timestamp array II for DFS, records when the search finishes exaining the adjacency list for each vertex
public: // member functions
	LinkedBase(int nVer);				//Initializer
	void delAllVer(void);				//delete all elements in the adjacency list
	int getVerNum(void);				//return number of vertices of graph
	int getEdgNum(void);				//return number of edges of graph
	int getOutDeg(int index);			//return the out degree of given vertex
	void initializePos(void);			//initialize iterPos
	void deactivatePos(void);			//delete iterPos
	int beginVertex(int index);			//return first vertex connected to given vertex
	int nextVertex(int index);			//return next vertex connected to given vertex
	void BFS(int s);					//breadth-first search (s is source vertex)
	void DFS1(void);					//depth-first search, creates depth-first forest
	void DFS2(int s);					//creates a depth-first tree with a specificied root - connected graph only
	void DFSVisit(int u);				//visits vertices in depth first search and marks them as visited; u is a vertex
	void copy(LinkedBase *graph1);		//copy a graph, but only vertices, edges, and adjacency.  Properties for iterator and graph traversal excluded
	void addAIsoVer(int n);				//add n isolated vertices into a graph
};

//LinkedDigraph class
class LinkedDigraph : public LinkedBase
{
public:
	inline LinkedDigraph(int nVer):LinkedBase(nVer){};		//Constructor link
	bool isEdgeExisting(int iIndex, int jIndex);			//determine if edge <i,j> esists in graph 
	void addEdge(int iIndex, int jIndex);					//add a new edge into the graph
	void deleteEdge(int iIndex, int jIndex);				//delete an existing edge from graph
	int getInDeg(int index);								//return the in degree of the given vertex
	int getOutDeg(int index);								//return the out degree of the given vertex
};

//LinkedUndigraph class
class LinkedUndigraph : public LinkedDigraph
{
public:
	inline LinkedUndigraph(int nVer):LinkedDigraph(nVer){};	//Constructor

	void addEdge(int iIndex, int jIndex);								//add a new edge into the graph
	void deleteEdge(int iIndex, int jIndex);							//delete an existing edge from graph
	int getInDeg(int index);											//return the in degree of the given vertex
	int getOutDeg(int index);											//return the out degree of the given vertex
	int getDeg(int index);												//Return the degree of the given vertex

	////the following functions support restoration
	void findFunCutSet(CHORDSET *chordSet, BRANCHVERTICES *tBranch, CHORDSET *cutSet);						//find fundamental cut set.
	void simplify(CHORDSET *chordset, BRANCHVERTICES *fBranch, int source, INTVECT *fVer, INTVECT *vMap);	//graph simplification
	//void unique_int(INTVECT *inputvect, INTVECT *outputvect);										//Outputvect is unique elements of input vector (sorted)
	void find_int(INTVECT *inputvect, INTVECT *foundvect, int findval, int numfind);				//foundvect is the matching entries of inputvect
	//void merge_sort_int(int *Input_Array, unsigned int Alen, int *Work_Array);						//Merge sort - stolen from solver_nr
	int isolateDeg12Ver(INTVECT *iVer);																//isolate vertices with degree < 3
	void deleteIsoVer(INTVECT *vMap);																//delete all isolated vertices (degree = 0)
	void mergeVer_2(INTVECT *vMap);																	//merge vertices according to the map of vertices

};

class restoration : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	int reconfig_attempts;				//Number of reconfigurations/timestep to try before giving up
	int reconfig_iter_limit;			//Number of iterations to let PF go before flagging this as a bad reconfiguration

	OBJECT *sourceVerObj;				//Object reference to source vertex
	OBJECT *faultSecObj;				//Object reference to faulted section

	char1024 fVerPub;					//Published string for fVer object list (to be parsed)
	char1024 feeder_power_limit_Pub;	//Published string for feeder_power_limit (to be parsed)
	char1024 feeder_power_link_Pub;		//Published string for feeder_power_link (to be parsed)

	char1024 mVerPub;					//Published string for mVer object list (to be parsed)
	char1024 microgrid_limit_Pub;		//Published string for microgrid_limit (to be parsed)
	char1024 microgrid_power_link_Pub;	//Published string for microgrid_power_link (to be parsed)

	char1024 logfile_name;				//Filename for output file of restoration -- if desired

	double voltage_limit[2];			//Lower and upper limits of load voltages

	bool stop_and_generate;				//Flag to either perform the base-WSU functionality (check all scenarios), or to just do a "first solution exit" approach
										//False = GLD approach (exit when first valid reconfig found), true = WSU MATLAB (generate all)

	//I/O functions for GLD Interface
	int PerformRestoration(int faulting_link);	//Base function - similar to main class of MATLAB (called by fault_check)

	restoration(MODULE *mod);
	inline restoration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

private:
	int s_ver;							//Source vertex - only one specifiable (saying so)
	int s_ver_1;						//Source vertex after first simpliciation
	int s_ver_2;						//Source vertex after the second simplification

	int numfVer;						//Size of feeder vertices
	INTVECT feederVertices;				//ID of the start vertices of feeders in the graph after the 2nd simplification
	INTVECT feederVertices_1;			//First reduction working version
	INTVECT feederVertices_2;			//Second reduction working verstion
	OBJECT **fVerObjList;				//Feeder vertices - object list
	OBJECT **fLinkObjList;				//Feeder "power link" - object list
	FUNCTIONADDR *fLinkPowerFunc;		//Feeder "power update" function list (matches fLinkObjList)
	double *feeder_power_limit;			//Feeder power limits - must match number of feeder vertices (feeders) - parsed from string
	gld::complex **feeder_power_link_value;	//Feeder power output - linked from parsed feeder list

	int numMG;							//Number of microgrids
	INTVECT MGIdx;						//Microgrid vertices - specify by object name and parse (string)
	INTVECT MGIdx_1;					//Microgrid vertices -- first reduction working version
	INTVECT MGIdx_2;					//Microgrid vertices -- second reduction working version
	OBJECT **mVerObjList;				//Microgrid vertices - object list
	OBJECT **mLinkObjList;				//Microgrid "power link" - object list
	FUNCTIONADDR *mLinkPowerFunc;		//Microgrid "power update" function list (matches fLinkObjList)
	gld::complex *microgrid_limit;			//Microgrid power limit - must match number of microgrid vertices - parsed from string (real & reactive)
	gld::complex **microgrid_power_link_value;	//Microgrid power output - linked from parsed microgrid list
	
	FUNCTIONADDR fault_check_fxn;		//Function address for "reliability_alterations" function - meant to do topology check after a restoration operation

	bool file_output_desired;			//Flag to see if a text file output is wanted

	LinkedUndigraph *top_ori;			//Graph object for original network
	LinkedUndigraph *top_sim_1;			//Graph object for topology after first simplification -- all non-switch edges deleted
	LinkedUndigraph *top_sim_2;			//Graph object for topology after second simplification -- vertices whose degrees are 1 or 2 are deleted
	LinkedUndigraph *top_res;			//Topology after restoration, represented by a tree
	LinkedUndigraph *top_tmp;			//Assumed to be a temporary tree/graph

	CHORDSET tie_swi;					//Set of tie-switches (nromally open), n*2 matrix
	CHORDSET tie_swi_1;					//Tie switches after the 1st simplification
	CHORDSET tie_swi_2;					//Tie switches after the 2nd simplification
	CHORDSET sec_swi;					//Set of sectionalizing switches (normally closed), n*2 matrix
	CHORDSET sec_swi_1;					//Sectionalizing switches after the 1st simplification
	CHORDSET sec_swi_2;					//Sectionalizing switches after the 2nd simplification
	CHORDSET sec_list;					//Copy of sec_swi for iterations -- not sure if really needed

	CHORDSET sec_swi_map;				//???
	CHORDSET sec_swi_map_1;				//???

	LOCSET tie_swi_loc;					//Location of tie switches status in original GLM file
	LOCSET sec_swi_loc;					//Location of sectionalizing switches status in original GLM file

	BRANCHVERTICES f_sec;				//Faulted section
	BRANCHVERTICES f_sec_1;				//Faulted section after 1st simplification
	BRANCHVERTICES f_sec_2;				//Faulted section after 2nd simplification

	INTVECT ver_map_1;					//Map between the vertices of the original tree and those of the 1st simplified tree
	INTVECT ver_map_2;					//Map between the vertices of the 1st simplification tree and the 2nd simplified tree

	CANDSWOP candidateSwOpe;			//Candidate switching operations
	CANDSWOP candidateSwOpe_1;			//Candidate switching operations on top_sim_1
	CANDSWOP candidateSwOpe_2;			//Candidate switching operations on top_sim_2

	gld::complex **voltage_storage;			//Voltage storage - to restore when powerflow dies a horrible death

	//Voltage saving (value saving) functions
	void PowerflowSave(void);
	void PowerflowRestore(void);

	//General parsing functions
	double *ParseDoubleString(char *input_string,int *num_items_found);
	gld::complex *ParseComplexString(char *input_string,int *num_items_found);
	OBJECT **ParseObjectString(char *input_string,int *num_items_found);
	bool isSwitch(int iIndex, int jIndex);
	bool isSwiInFeeder(BRANCHVERTICES *swi_2, int feederID_overload);

	//General output function
	void printResult(int IdxSW);
	void printSOs(FILE *FPHandle, int IdxSW);
	void printCIO(FILE *FPHandle, int IdxSW);

	//General allocation algorithms -- functionalized just for simplicity
	void INTVECTalloc(INTVECT *inititem, int allocsizeval);
	void CHORDSETalloc(CHORDSET *inititem, int allocsizeval);
	void LOCSETalloc(LOCSET *inititem, int allocsizeval);
	void CANDSWOPalloc(CANDSWOP *inititem, int allocsizeval);

	//General deallocation functions - again for simplicity
	void INTVECTfree(INTVECT *inititem);
	void CHORDSETfree(CHORDSET *inititem);
	void LOCSETfree(LOCSET *inititem);
	void CANDSWOPfree(CANDSWOP *inititem);

	//Reconfiguration functions
	void simplifyTop_1(void);
	//void loadInfo(void);
	void simplifyTop_2(void);
	void setFeederVertices_2(void);
	void setMicrogridVertices(void);
	void modifySecSwiList(void);
	void formSecSwiMap(void);
	void mapSecSwi(BRANCHVERTICES *sSW_2, BRANCHVERTICES *sSW, BRANCHVERTICES *sSW_1);
	void mapTieSwi(BRANCHVERTICES *tSW_2, BRANCHVERTICES *tSW, BRANCHVERTICES *tSW_1);
	void renewFaultLocation(BRANCHVERTICES *faultsection);
	int spanningTreeSearch(void);
	void CHORDSETintersect(CHORDSET *set_1, CHORDSET *set_2, CHORDSET *intersect);
	void modifyModel(int counter);
	int runPowerFlow(void);
	void checkPF2(bool *flag, double *overLoad, int *feederID);
	bool checkVoltage(void);
	void checkFeederPower(bool *fFlag, double *overLoad, int *feederID);
	void checkMG(bool *mFlag, double *overLoad, int *microgridID);
	void checkLinePower(bool *lFlag, double *overLoad, int *lineID);
};

EXPORT int perform_restoration(OBJECT *thisobj,int faulting_link);

void unique_int(INTVECT *inputvect, INTVECT *outputvect);										//Outputvect is unique elements of input vector (sorted)
void merge_sort_int(int *Input_Array, unsigned int Alen, int *Work_Array);						//Merge sort - stolen from solver_nr

#endif // _RESTORATION_H
