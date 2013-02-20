# find . -iname "*.c*" | grep -v "./tests" | grep -v "/swig" | grep -v "/php" | grep -v "cmake" | grep -v "/doc"

# This script is used to update the version of GEOS included here. Since some files
# have the same name and it's compiled into one library, the basenames can't conflict.

mv geos/src/geomgraph/DirectedEdgeStar.cpp geos/src/geomgraph/GeomGraphDirectedEdgeStar.cpp
mv geos/src/geomgraph/NodeMap.cpp geos/src/geomgraph/GeomGraphNodeMap.cpp
mv geos/src/geomgraph/index/SweepLineEvent.cpp geos/src/geomgraph/index/GeomGraphSweepLineEvent.cpp
mv geos/src/geomgraph/EdgeRing.cpp geos/src/geomgraph/GeomGraphEdgeRing.cpp
mv geos/src/geomgraph/DirectedEdge.cpp geos/src/geomgraph/GeomGraphDirectedEdge.cpp
mv geos/src/geomgraph/Node.cpp geos/src/geomgraph/GeomGraphNode.cpp
mv geos/src/index/bintree/Root.cpp geos/src/index/bintree/BinTreeRoot.cpp
mv geos/src/index/bintree/Interval.cpp geos/src/index/bintree/BinTreeInterval.cpp
mv geos/src/geomgraph/Edge.cpp geos/src/geomgraph/GeomGraphEdge.cpp
mv geos/src/geomgraph/PlanarGraph.cpp geos/src/geomgraph/GeomGraphPlanarGraph.cpp
mv geos/src/index/bintree/NodeBase.cpp geos/src/index/bintree/BinTreeNodeBase.cpp
mv geos/src/index/bintree/Key.cpp geos/src/index/bintree/BinTreeKey.cpp
mv geos/src/index/bintree/Node.cpp geos/src/index/bintree/BinTreeNode.cpp
mv geos/src/index/quadtree/Node.cpp geos/src/index/quadtree/QuadTreeNode.cpp
