#ifndef NODEMASK_H
#define NODEMASK_H

#include <QString>

namespace osg {
class Node;
}
class QRegExpValidator;

class  NodeMask
{
private:
    NodeMask() {}
public:
    static const unsigned ChildShift = 16;
    // Node masks.  Note that for groups we also set whether there exists a child
    // with each bit set by shifting the bits up two bytes.  So a node with the
    // SELECTED bit set (0x02) should have a parent with the (0x020000) bit set.  This
    // allows the OSG cullMask to select drawing "selected items" by traversing the
    // graph with a cullMask of 0x0202.
    enum NodeMaskValue {
        NONE = 0,
        UNSELECTED = 1<<0,
        SELECTED = 1<<1,
        HIDDEN = 1<<2,
        SHOTLINE = 1<<7,
        GROUP = 1<<8,
        AXIS = 1<<9,
        CUTTINGPLANE = 1<<10,
        POINTINDICATOR = 1<<11,
        ALL = ~0 // camera cullMask always has this set so we traverse to leaves
                        // Group nodes should have it set, only Geode/Geometry need not.
    };

    static QString maskToString(unsigned mask);
    static unsigned stringToMask(QString s);
    static void setNodeMasksOnHeirarchy(osg::Node *n, NodeMaskValue mask);
    static void setNodeMasksBitOnHeirarchy(osg::Node *n, NodeMaskValue mask);
    static void clearNodeMasksBitOnHeirarchy(osg::Node *n, NodeMaskValue mask);
    static void markGroups(osg::Node *n);
    static void markGroupsAndLeafNodes(osg::Node *n, NodeMaskValue mask);
    static QRegExpValidator * createValidator();
    static const int bitsForDisplayMask = 12;
    static const char * const yadda;

    static const char * const bitNames[];
};



#endif // NODEMASK_H
