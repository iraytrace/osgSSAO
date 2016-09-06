#include "NodeMask.h"
#include <QDebug>
#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/Drawable>
#include <bitset>
//static const int bitsForDisplayMask = 12;
static const unsigned defaultMask = 0x0fff;

const char * const NodeMask::yadda = "hello";

const char * const NodeMask::bitNames[] = {
    "Unselected",   // 0
        "Selected",     // 1
        "Hidden",       // 2
        "",             // 3
        "",             // 4
        "",             // 5
        "",             // 6
        "Shotline",     // 7
        "Group",        // 8
        "Axis",         // 9
        "CuttingPlane", // 10
        "PointIndicator"// 11
};
class GroupLeafNodeSetVisitor : public osg::NodeVisitor
{
public:
    GroupLeafNodeSetVisitor(unsigned int mask=defaultMask,
                            TraversalMode tm=TRAVERSE_ALL_CHILDREN)
        : NodeVisitor(tm)
        , m_mask(mask)
    { // force traversal of all nodes
        _traversalMask = _nodeMaskOverride = ~0;
    }
    virtual void apply (osg::Group &node) {
        node.setNodeMask(NodeMask::GROUP|m_mask);
        traverse(node);
    }
    virtual void apply (osg::Drawable &node) {
        node.setNodeMask(m_mask);
        this->traverse(node);
    }
private:
    unsigned m_mask;
};

/// Assign a nodemask to nodes
/// This overrides any nodeMask that has been set entirely
class NodeMaskSetVisitor : public osg::NodeVisitor
{
public:
    NodeMaskSetVisitor(unsigned int mask=defaultMask,
                      TraversalMode tm=TRAVERSE_ALL_CHILDREN)
        : NodeVisitor(tm)
        , m_mask(mask)
    { // force traversal of all nodes
        _traversalMask = _nodeMaskOverride = ~0;
    }

    virtual void apply (osg::Node &node) {
        node.setNodeMask(m_mask);
        this->traverse(node);
    }
private:
    unsigned m_mask;
};


/// Set individual bits of the NodeMask in a tree by doing an OR with the
/// provided bitmask.   This is usefull for turning on additional bits in
/// the nodemask
class NodeMaskOrBitVisitor : public osg::NodeVisitor
{
public:
    NodeMaskOrBitVisitor(unsigned int mask=defaultMask,
                      TraversalMode tm=TRAVERSE_ALL_CHILDREN)
        : NodeVisitor(tm)
        , m_mask(mask)
    { // force traversal of all nodes
        _traversalMask = _nodeMaskOverride = ~0;
    }

    virtual void apply (osg::Node &node) {
        this->traverse(node);
        unsigned int before = node.getNodeMask();
        unsigned int after = before | m_mask;
        node.setNodeMask(after);
    }
private:
    unsigned m_mask;
};


/// Clear individual bits of the nodemask in a tree by doing
/// and AND with the inverse of the bits set in the mask provided.
class NodeMaskAndBitVisitor : public osg::NodeVisitor
{
public:
    NodeMaskAndBitVisitor(unsigned int mask=defaultMask,
                      TraversalMode tm=TRAVERSE_ALL_CHILDREN)
        : NodeVisitor(tm)
        , m_mask(mask)
    { // force traversal of all nodes
        _traversalMask = _nodeMaskOverride = ~0;
    }

    virtual void apply (osg::Node &node) {
        this->traverse(node);
        unsigned int before = node.getNodeMask();
        unsigned int after = before & m_mask;
        node.setNodeMask(after);
    }
private:
    unsigned m_mask;
};

QString NodeMask::maskToString(unsigned mask)
{
    std::bitset<bitsForDisplayMask> bits(mask);
    QString s = QString::fromStdString(bits.to_string());

    s.insert(8, ' ');
    s.insert(4, ' ');
    return s;
}

unsigned NodeMask::stringToMask(QString s)
{
    s.remove(' ');
    std::bitset<bitsForDisplayMask> bits(s.toStdString());
    return (unsigned)bits.to_ulong();
}

void NodeMask::setNodeMasksOnHeirarchy(osg::Node *n, NodeMaskValue mask)
{
    NodeMaskSetVisitor nmsv(mask);

    n->accept(nmsv);
}

void NodeMask::setNodeMasksBitOnHeirarchy(osg::Node *n, NodeMaskValue mask)
{
    unsigned settingMask = mask & 0x0ff;

    NodeMaskOrBitVisitor nmbsv(settingMask);
    n->accept(nmbsv);
}

void NodeMask::clearNodeMasksBitOnHeirarchy(osg::Node *n, NodeMaskValue mask)
{
    unsigned clearingMask = ~(mask & 0x0ff);

    NodeMaskAndBitVisitor nmcbv(clearingMask);
    n->accept(nmcbv);
}

void NodeMask::markGroups(osg::Node *n)
{
    GroupLeafNodeSetVisitor gv;
    n->accept(gv);
}

void NodeMask::markGroupsAndLeafNodes(osg::Node *n, NodeMask::NodeMaskValue mask)
{
    GroupLeafNodeSetVisitor gv(mask);
    n->accept(gv);
}

#include <QRegExpValidator>
QRegExpValidator *NodeMask::createValidator()
{
    return new QRegExpValidator(QRegExp("([01]* )*[01]+"));
}
