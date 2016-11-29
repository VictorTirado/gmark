/*
 * incrementalDeterministicGraphGenerator.cpp
 *
 *  Created on: Sep 16, 2016
 *      Author: wilcovanleeuwen
 */

#include <vector>
#include "incrementalDeterministicGraphGenerator.h"
#include "graphNode.h"

namespace std {

incrementalDeterministicGraphGenerator::incrementalDeterministicGraphGenerator() {
	randomGenerator.seed(chrono::system_clock::now().time_since_epoch().count());
}

incrementalDeterministicGraphGenerator::~incrementalDeterministicGraphGenerator() {
	// TODO Auto-generated destructor stub
}

// ####### Generate edges #######
void incrementalDeterministicGraphGenerator::addEdge(graphNode &sourceNode, graphNode &targetNode, int predicate) {
	sourceNode.decrementOpenInterfaceConnections();
	targetNode.decrementOpenInterfaceConnections();

	edge2 newEdge;
	newEdge.subjectId = sourceNode.id;
	newEdge.predicate = to_string(predicate);
	newEdge.objectId = targetNode.id;

	edges.push_back(newEdge);
}
// ####### Generate edges #######


// ####### Update interface-connections #######
void incrementalDeterministicGraphGenerator::updateInterfaceConnectionsForZipfianDistributions(vector<graphNode> *nodesVec, distribution distr, bool outDistr) {
//	cout << "New Zipfian case" << endl;
	int nmNodes = nodesVec->size();

	vector<double> zipfianCdf = cumDistrUtils.zipfCdf(distr.arg2, nmNodes);

	int shift = 0;
	if (outDistr) {
		shift = outDistrShift;
	} else {
		shift = inDistrShift;
	}

	int newInterfaceConnections = 0;
	int difference = 0;
	for (graphNode & node: *nodesVec) {
		newInterfaceConnections = cumDistrUtils.findPositionInCdf(zipfianCdf, node.getPosition()) + shift;

//		cout << "newInterfaceConnections: " << newInterfaceConnections << endl;
		difference = newInterfaceConnections - node.getNumberOfInterfaceConnections();
		node.incrementOpenInterfaceConnectionsByN(difference);
		node.setNumberOfInterfaceConnections(newInterfaceConnections);
//		cout << "after openICs: " << node.getNumberOfOpenInterfaceConnections() << endl;
	}
}


void incrementalDeterministicGraphGenerator::performShiftForNonScalableNodes(config::edge & edgeType) {
	// Both distribution are non-Zipfian

	if (!conf.types.at(edgeType.subject_type).scalable) {
		double meanOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, -1) + outDistrShift;
		double meanInDistr = getMeanICsPerNode(edgeType.incoming_distrib, -1) + inDistrShift;
		int newMeanOutDistr = floor((conf.types.at(edgeType.object_type).size * meanInDistr) / conf.types.at(edgeType.subject_type).size);
		int incr = newMeanOutDistr - meanOutDistr;

		if (incr > 0) {
			cout << "Shift out-distrib with: " << incr << endl;
			outDistrShift += incr;
			for (graphNode & n: nodes.first) {
				n.incrementInterfaceConnectionsByN(incr);
				n.incrementOpenInterfaceConnectionsByN(incr);
			}
		}
	} else {
		double meanOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, -1) + outDistrShift;
		double meanInDistr = getMeanICsPerNode(edgeType.incoming_distrib, -1) + inDistrShift;
		int newMeanInDistr = floor((conf.types.at(edgeType.subject_type).size * meanOutDistr) / conf.types.at(edgeType.object_type).size);
		int incr = newMeanInDistr - meanInDistr;

		if (incr > 0) {
			cout << "Shift in-distrib with: " << incr << endl;
			inDistrShift += incr;
			for (graphNode & n: nodes.second) {
				n.incrementInterfaceConnectionsByN(incr);
				n.incrementOpenInterfaceConnectionsByN(incr);
			}
		}
	}
}
// ####### Update interface-connections #######

double incrementalDeterministicGraphGenerator::getMeanICsPerNode(distribution & distr, int zipfMax) {
	double meanEdgesPerNode;
	if (distr.type == DISTRIBUTION::NORMAL) {
		meanEdgesPerNode = distr.arg1;
	} else if (distr.type == DISTRIBUTION::UNIFORM) {
		meanEdgesPerNode = (distr.arg2 + distr.arg1) / 2.0;
	} else if (distr.type == DISTRIBUTION::ZIPFIAN) {
		double temp = 0.0;
		for (int i=1; i<=zipfMax; i++) {
			temp += (i-1)*pow((i), -1*distr.arg2);
		}
		meanEdgesPerNode = temp;
		cout << "Zipfian mean=" << meanEdgesPerNode << endl;
		cout << "####----###### DIT HAD NOOIT MOGEN GEBEUREN!!!! ####----######" << endl;
	} else {
		meanEdgesPerNode = 1;
	}
	return meanEdgesPerNode;
}


void incrementalDeterministicGraphGenerator::fixSchemaInequality(config::edge & edgeType) {
	cout << "conf.types[edgeType.subject_type].size: " << conf.types[edgeType.subject_type].size << endl;
	cout << "conf.types[edgeType.object_type].size: " << conf.types[edgeType.object_type].size << endl;

	double evOutDistribution = getMeanICsPerNode(edgeType.outgoing_distrib, conf.types[edgeType.subject_type].size) + outDistrShift;
	cout << "evOutDistribution: " << evOutDistribution << endl;

	double evInDistribution = getMeanICsPerNode(edgeType.incoming_distrib, conf.types[edgeType.object_type].size) + inDistrShift;
	cout << "evInDistribution: " << evInDistribution << endl;

	if ((conf.types[edgeType.subject_type].size * evOutDistribution) > (conf.types[edgeType.object_type].size * evInDistribution)) {
		double newMean = ((double) conf.types[edgeType.subject_type].size * evOutDistribution) / (double) conf.types[edgeType.object_type].size;
		int updateInt = floor(newMean - evInDistribution);

		cout << "newMean in: " << newMean << endl;
		if (updateInt > 0) {
			inDistrShift += updateInt;
			cout << "Update in distribution: " << updateInt << endl;
			cout << "inDistrShift: " << inDistrShift << endl;
			for (graphNode & n: nodes.second) {
				n.incrementInterfaceConnectionsByN(updateInt);
				n.incrementOpenInterfaceConnectionsByN(updateInt);
			}
		}
	} else {
		double newMean = ((double) conf.types[edgeType.object_type].size * evInDistribution) / (double) conf.types[edgeType.subject_type].size;
		int updateInt = floor(newMean - evOutDistribution);

		cout << "newMean out: " << newMean << endl;
		if (updateInt > 0) {
			outDistrShift += updateInt;
			cout << "Update out distribution: " << updateInt << endl;
			cout << "outDistrShift: " << outDistrShift << endl;
			for (graphNode & n: nodes.first) {
				n.incrementInterfaceConnectionsByN(updateInt);
				n.incrementOpenInterfaceConnectionsByN(updateInt);
			}
		}
	}
}


vector<int> constructNodesVector(vector<graphNode> & nodes) {
	vector<int> nodesVector;
	for (graphNode & n: nodes) {
//		cout << "node" << n.iterationId << " has " << n.getNumberOfOpenInterfaceConnections() << " openICs" << endl;
		for (int i=0; i<n.getNumberOfOpenInterfaceConnections(); i++) {
			nodesVector.push_back(n.iterationId);
		}
	}
	return nodesVector;
}

vector<int> constructNodesVectorMinusOne(vector<graphNode> & nodes) {
	vector<int> nodesVector;
	for (graphNode & n: nodes) {
//		cout << "node" << n.iterationId << " has " << n.getNumberOfOpenInterfaceConnections() << " openICs" << endl;
		for (int i=0; i<n.getNumberOfOpenInterfaceConnections()-1; i++) {
			nodesVector.push_back(n.iterationId);
		}
	}
	return nodesVector;
}

vector<int> constructNodesVectorLastOne(vector<graphNode> & nodes) {
	vector<int> nodesVector;
	for (graphNode & n: nodes) {
		if(n.getNumberOfOpenInterfaceConnections() > 0) {
			nodesVector.push_back(n.iterationId);
		}
	}
	return nodesVector;
}

void incrementalDeterministicGraphGenerator::performSchemaIndicatedShift(config::edge & edgeType) {
	int sf = edgeType.scale_factor;
	if (sf <= 0) {
		return;
	}

	if (conf.types[edgeType.subject_type].size > conf.types[edgeType.object_type].size) {
		// Increment all ICs and openICs with 1
		outDistrShift += sf;
		cout << "Update out-nodes in dbsh: " << sf << endl;
		for (graphNode & node: nodes.first) {
			node.incrementOpenInterfaceConnectionsByN(sf);
			node.incrementInterfaceConnectionsByN(sf);
		}

		// Increment all ICs and openICs with floor(num) or floor(num)+1 with probability num-floor(num)
		double num = ((double) sf * (double)conf.types[edgeType.subject_type].size) / (double)conf.types[edgeType.object_type].size;
		int numIncrement = floor(num);
		double randomValue = uniformDistr(randomGenerator);
		if (randomValue < num - (double)floor(num)) {
			numIncrement++;
		}
		inDistrShift += numIncrement;
		cout << "Update in-nodes in dbsh: " << sf << endl;
		for (graphNode & node: nodes.second) {
			node.incrementOpenInterfaceConnectionsByN(numIncrement);
			node.incrementInterfaceConnectionsByN(numIncrement);
		}
	} else {
		// Increment all ICs and openICs with 1
		inDistrShift += sf;
		for (graphNode & node: nodes.second) {
			cout << "Update out-nodes in dbsh: " << sf << endl;
			node.incrementOpenInterfaceConnectionsByN(sf);
			node.incrementInterfaceConnectionsByN(sf);
		}

		// Increment all ICs and openICs with floor(num) or floor(num)+1 with probability num-floor(num)
		double num = ((double) sf * (double)conf.types[edgeType.object_type].size) / (double)conf.types[edgeType.subject_type].size;
		int numIncrement = floor(num);
		double randomValue = uniformDistr(randomGenerator);
		if (randomValue < num - (double)floor(num)) {
			numIncrement++;
		}
		outDistrShift += numIncrement;
		for (graphNode & node: nodes.first) {
			cout << "Update in-nodes in dbsh: " << sf << endl;
			node.incrementOpenInterfaceConnectionsByN(numIncrement);
			node.incrementInterfaceConnectionsByN(numIncrement);
		}
	}
}

void incrementalDeterministicGraphGenerator::performFixingShiftForZipfian(config::edge & edgeType, vector<int> & subjectNodeIdVector, vector<int> & objectNodeIdVector) {
	// Perform a shift in one of the distributions, when there is a large difference in the lengths of the vectors
	if (subjectNodeIdVector.size() < objectNodeIdVector.size()) {
		int diff = objectNodeIdVector.size() - subjectNodeIdVector.size();
		int incr = diff / conf.types[edgeType.subject_type].size;
		if (incr > 0) {
			cout << "Shift out-distr after vector gen: " << incr << endl;
			outDistrShift += incr;
			cout << "outDistrShift: " << outDistrShift << endl;
			for (graphNode & n: nodes.first) {
				n.incrementInterfaceConnectionsByN(incr);
				n.incrementOpenInterfaceConnectionsByN(incr);
				for (int i=0; i<incr; i++) {
					subjectNodeIdVector.push_back(n.iterationId);
				}
			}
		}
	} else {
		int diff = subjectNodeIdVector.size() - objectNodeIdVector.size();
		int incr = diff / conf.types[edgeType.object_type].size;
		if (incr > 0) {
			cout << "Shift in-distr after vector gen: " << incr << endl;
			inDistrShift += incr;
			cout << "inDistrShift: " << inDistrShift << endl;
			for (graphNode & n: nodes.second) {
				n.incrementInterfaceConnectionsByN(incr);
				n.incrementOpenInterfaceConnectionsByN(incr);
				for (int i=0; i<incr; i++) {
					objectNodeIdVector.push_back(n.iterationId);
				}
			}
		}
	}
}

void incrementalDeterministicGraphGenerator::incrementGraph(config::edge & edgeType, int graphNumber) {
	// Perform the shifting of the distribution as indecated by the user in the schema

	if (graphNumber > 0) {
		performSchemaIndicatedShift(edgeType);
		if ((conf.types.at(edgeType.subject_type).scalable ^ conf.types.at(edgeType.object_type).scalable) &&
					 edgeType.outgoing_distrib.type != DISTRIBUTION::ZIPFIAN &&
					 edgeType.incoming_distrib.type != DISTRIBUTION::ZIPFIAN) {
			performShiftForNonScalableNodes(edgeType);
		}
	}


	// Add subject and object nodes to the graph
	// Specify the current shift to get the wright amound of ICs
	nodeGen.addSubjectNodes(edgeType, outDistrShift);
	nodeGen.addObjectNodes(edgeType, inDistrShift);



	// Update the ICs for the Zipfian distribution to satisfy the property that influecer nodes will get more ICs when the graph grows
	if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN) {
		updateInterfaceConnectionsForZipfianDistributions(&nodes.first, edgeType.outgoing_distrib, true);
	}
	if (edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
		updateInterfaceConnectionsForZipfianDistributions(&nodes.second, edgeType.incoming_distrib, false);
	}


	if (edgeType.scale_factor > 0 ||
			!conf.types[edgeType.subject_type].scalable || !conf.types[edgeType.object_type].scalable ||
			edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN || edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
		vector<int> subjectNodeIdVector = constructNodesVector(nodes.first);
		vector<int> objectNodeIdVector = constructNodesVector(nodes.second);

		if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN || edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
			performFixingShiftForZipfian(edgeType, subjectNodeIdVector, objectNodeIdVector);
		}

		shuffle(subjectNodeIdVector.begin(), subjectNodeIdVector.end(), randomGenerator);
		shuffle(objectNodeIdVector.begin(), objectNodeIdVector.end(), randomGenerator);


		for (int i=0; i<min(subjectNodeIdVector.size(), objectNodeIdVector.size()); i++) {
			addEdge(nodes.first[subjectNodeIdVector[i]], nodes.second[objectNodeIdVector[i]], edgeType.predicate);
		}
	} else {
		// Create vectors
		vector<int> subjectNodeIdVectorMinusOne = constructNodesVectorMinusOne(nodes.first);
		vector<int> objectNodeIdVectorMinusOne = constructNodesVectorMinusOne(nodes.second);

		vector<int> subjectNodeIdVectorLastOne = constructNodesVectorLastOne(nodes.first);
		vector<int> objectNodeIdVectorLastOne = constructNodesVectorLastOne(nodes.second);

		// Shuffle small vectors
		shuffle(subjectNodeIdVectorLastOne.begin(), subjectNodeIdVectorLastOne.end(), randomGenerator);
		shuffle(objectNodeIdVectorLastOne.begin(), objectNodeIdVectorLastOne.end(), randomGenerator);

		// Add the difference in the lastOne-vector to the minusOne-vector
		if (subjectNodeIdVectorLastOne.size() > objectNodeIdVectorLastOne.size()) {
			for (int i=objectNodeIdVectorLastOne.size(); i<subjectNodeIdVectorLastOne.size(); i++) {
				subjectNodeIdVectorMinusOne.push_back(subjectNodeIdVectorLastOne[i]);
			}
		} else {
			for (int i=subjectNodeIdVectorLastOne.size(); i<objectNodeIdVectorLastOne.size(); i++) {
				objectNodeIdVectorMinusOne.push_back(objectNodeIdVectorLastOne[i]);
			}
		}

		if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN || edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
			performFixingShiftForZipfian(edgeType, subjectNodeIdVectorMinusOne, objectNodeIdVectorMinusOne);
		}


		// Shuffle large vectors
		shuffle(subjectNodeIdVectorMinusOne.begin(), subjectNodeIdVectorMinusOne.end(), randomGenerator);
		shuffle(objectNodeIdVectorMinusOne.begin(), objectNodeIdVectorMinusOne.end(), randomGenerator);


		// Create edges
		for (int i=0; i<min(subjectNodeIdVectorMinusOne.size(), objectNodeIdVectorMinusOne.size()); i++) {
			addEdge(nodes.first[subjectNodeIdVectorMinusOne[i]], nodes.second[objectNodeIdVectorMinusOne[i]], edgeType.predicate);
		}

		for (int i=0; i<min(subjectNodeIdVectorLastOne.size(), objectNodeIdVectorLastOne.size()); i++) {
			double randomValue = uniformDistr(randomGenerator);
			if (randomValue > 0.25) {
				addEdge(nodes.first[subjectNodeIdVectorLastOne[i]], nodes.second[objectNodeIdVectorLastOne[i]], edgeType.predicate);
			}
		}
	}
}

void incrementalDeterministicGraphGenerator::printRankZipf(vector<graphNode> nodes, int edgeTypeId, int nbNodes) {
	int maxDegree = 0;
	for (graphNode node: nodes) {
		int degree = node.numberOfInterfaceConnections - node.numberOfOpenInterfaceConnections;
		if (degree > maxDegree) {
			maxDegree = degree;
		}
	}

	cout << "Maxdegree=" << maxDegree << endl;
	ofstream rankFile, degreeFile;
	rankFile.open("rankFileET" + to_string(edgeTypeId) + "n" + to_string(nbNodes) + ".txt", ios::trunc);
	degreeFile.open("rankFileET" + to_string(edgeTypeId) + "n" + to_string(nbNodes) + "degree.txt", ios::trunc);
	for (graphNode node: nodes) {
		double rank = ((double)node.numberOfInterfaceConnections - (double)node.numberOfOpenInterfaceConnections) / (double) maxDegree;
		rankFile << to_string(rank) << endl;
		degreeFile << to_string(node.numberOfInterfaceConnections - node.numberOfOpenInterfaceConnections) << "-" << to_string(maxDegree) << endl;
	}
	rankFile.close();
}

void incrementalDeterministicGraphGenerator::printRankNonZipf(vector<graphNode> nodes, int edgeTypeId, int nbNodes) {
	ofstream rankFile;
	rankFile.open("rankFileET" + to_string(edgeTypeId) + "n" + to_string(nbNodes) + ".txt", ios::trunc);
	for (graphNode currentNode: nodes) {
		int nodesWithLowerDegree = 0;
		int degree = currentNode.numberOfInterfaceConnections - currentNode.numberOfOpenInterfaceConnections;
		for (graphNode compareNode: nodes) {
			if (compareNode.numberOfInterfaceConnections - compareNode.numberOfOpenInterfaceConnections <= degree) {
				nodesWithLowerDegree++;
			}
		}
		double rank = (double)nodesWithLowerDegree / (double)nodes.size();
		rankFile << to_string(rank) << endl;
	}


	rankFile.close();
}


int incrementalDeterministicGraphGenerator::processEdgeTypeSingleGraph(config::config configuration, config::config previousConf, config::edge & edgeType, ofstream & outputFile, int graphNumber_) {
	chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

	this->conf = configuration;
	this->graphNumber = graphNumber_;
	cout << "\n\n-----------GraphNumber: " << graphNumber << "--------------" << endl;
//	cout << "Number of nodes: " << conf.nb_nodes << endl;

	if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN && outDistrShift == 0) {
		outDistrShift = 1;
	}
	if (edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN && inDistrShift == 0) {
		inDistrShift = 1;
	}

	if (graphNumber == 0 &&
			edgeType.outgoing_distrib.type != DISTRIBUTION::ZIPFIAN &&
			edgeType.incoming_distrib.type != DISTRIBUTION::ZIPFIAN) {
			fixSchemaInequality(edgeType);
	}

	nodeGen = nodeGenerator(edgeType, nodes.first.size(), nodes.second.size(), &randomGenerator, &nodes, &conf);


	cout << "OutShif: " << outDistrShift << endl;
	cout << "InShif: " << inDistrShift << endl;
//	int nbEdgesBeforeIncrementing = edges.size();
	incrementGraph(edgeType, graphNumber);

//	chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
//	auto durationWitoutMaterialize = chrono::duration_cast<chrono::milliseconds>( end - start ).count();

	// Materialize the edge
//	string outputBuffer = "";
//	cout << "Number of edges: " << edges.size() << endl;
	for (int i=0; i<edges.size(); i++) {
		edge2 e = edges[i];
		string edgeString = e.subjectId + " " + e.predicate + " " + e.objectId;

		if (i==edges.size()-1) {
			// Use endl to flush
//			outputBuffer += edgeString;
			outputFile << edgeString << endl;
//			outputBuffer = "";
		} else {
//			outputBuffer += edgeString + "\n";
			outputFile << edgeString << "\n";
		}
	}
	outputFile.flush();
	chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
	auto durationWithMaterialize = chrono::duration_cast<chrono::milliseconds>( end - start ).count();

//	// Number of nodes analysis
//	int subjectNodes = 0;
//	for (graphNode n: nodes.first) {
//		if (n.numberOfInterfaceConnections != n.numberOfOpenInterfaceConnections) {
//			subjectNodes++;
//		}
//	}
//	int objectNodes = 0;
//	for (graphNode n: nodes.second) {
//		if (n.numberOfInterfaceConnections != n.numberOfOpenInterfaceConnections) {
//			objectNodes++;
//		}
//	}
//
//	cout << "Subject nodes: " << subjectNodes << endl;
//	cout << "Object nodes: " << objectNodes << endl;

//	printRankZipf(nodes.first, edgeType.edge_type_id, conf.nb_nodes);
//	printRankNonZipf(nodes.first, edgeType.edge_type_id, conf.nb_nodes);
//	printRank(nodes.second, edgeType.edge_type_id, conf.nb_nodes);


	// Connectiveness Analysis
//	int newnew = 0;
//	int newOld = 0;
//	int oldold = 0;
//	for (int i=nbEdgesBeforeIncrementing-1; i<edges.size(); i++) {
//		string subjectId = edges[i].subjectId;
//		string subjectIdLocalId = subjectId.substr(subjectId.find("-")+1, subjectId.length());
//		int subjectIdInt = stoi(subjectIdLocalId);
////		cout << "subjectIdInt: " << subjectIdInt << endl;
//
//		string objectId = edges[i].objectId;
//		string objectIdLocalId = objectId.substr(objectId.find("-")+1, objectId.length());
//		int objectIdInt = stoi(objectIdLocalId);
////		cout << "objectIdInt: " << objectIdInt << endl;
//
//		if (subjectIdInt >= 500 && objectIdInt >= 500) {
//			newnew++;
//		} else if (subjectIdInt < 500 && objectIdInt < 500) {
//			oldold++;
//		} else {
//			newOld++;
//		}
//	}
//	cout << "NewNew: " << newnew << endl;
//	cout << "OldOld: " << oldold << endl;
//	cout << "NewOld: " << newOld << endl;
//	ofstream newnewF, oldoldF, newOldF;
//	newnewF.open("newnew.txt", ios::app);
//	oldoldF.open("oldold.txt", ios::app);
//	newOldF.open("newold.txt", ios::app);
//	if (graphNumber > 0) {
//		newnewF << newnew << ",";
//		oldoldF << oldold << ",";
//		newOldF << newOld << ",";
//	}

//	vector<int> connectedToOldGraph;
//	for (int i=nbEdgesBeforeIncrementing-1; i<edges.size(); i++) {
//		connectedToOldGraph.push_back(0);
//	}
//	for (int i=nbEdgesBeforeIncrementing-1; i<edges.size(); i++) {
//		string subjectId = edges[i].subjectId;
//		string subjectIdLocalId = subjectId.substr(subjectId.find("-")+1, subjectId.length());
//		int subjectIdInt = stoi(subjectIdLocalId);
////		cout << "subjectIdInt: " << subjectIdInt << endl;
//
//		string objectId = edges[i].objectId;
//		string objectIdLocalId = objectId.substr(objectId.find("-")+1, objectId.length());
//		int objectIdInt = stoi(objectIdLocalId);
////		cout << "objectIdInt: " << objectIdInt << endl;
//
//		if (subjectIdInt >= 500 && objectIdInt < 500) {
//			connectedToOldGraph[subjectIdInt-500]++;
//		} else if (subjectIdInt < 500 && objectIdInt >= 500) {
//			connectedToOldGraph[objectIdInt-500]++;
//		}
//	}
//
//	int nodesConnected = 0;
//	for(int c: connectedToOldGraph) {
//		if (c>0) {
//			nodesConnected++;
//		}
//	}
//	ofstream nConnected;
//	nConnected.open("nConnected.txt", ios::app);
//	if (graphNumber > 0) {
//		nConnected << nodesConnected << ",";
//	}

	return durationWithMaterialize;
}



} /* namespace std */
