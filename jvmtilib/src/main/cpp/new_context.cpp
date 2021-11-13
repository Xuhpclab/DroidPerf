#include <assert.h>
#include "new_context.h"


NewContextFrame &NewContextFrame::operator=(const ASGCT_CallFrame &asgct_frame) {
    method_id = asgct_frame.method_id;
    bci = asgct_frame.lineno;
    return *this;
}


NewContext::NewContext(uint32_t id):_id(id) {
    children = std::unordered_set<NewContext*, std::function<std::size_t(NewContext*)>, std::function<bool(NewContext*,NewContext*)> > (80/*bucket*/, contextRefHash, contextRefEqual);
}

NewContext::~NewContext() {
    if (_metrics){
        delete _metrics;
        _metrics = nullptr;
    }
}

NewContext& NewContext::operator=(const NewContextFrame &frame) {
    _frame = frame;
    return *this;
}

std::vector<NewContext *> NewContext::getChildren() {
    std::vector<NewContext *> ret_vec;
    for (const auto &c : children){
        ret_vec.push_back(c);
    }
    return ret_vec;
}

void NewContext::removeChild(NewContext *ctxt) {
    assert(false); //no one is really using this function
    children.erase(ctxt);
}

void NewContext::addChild(NewContext *ctxt) {
    // we need to update the child's parent
    children.insert(ctxt);
    ctxt->_parent = this;
}

NewContext *NewContext::findChild(NewContext *ctxt) {
    auto it = children.find(ctxt);
    if (it == children.end()) {
        return nullptr;
    } else {
        return (*it);
    }
}


namespace xml {
    template<>
    XMLObj *createXMLObj(NewContext *instance) {
        std::ostringstream os;
        XMLObj *obj = new(std::nothrow)XMLObj("context");
        assert(obj);
#define SET_ATTR(xmlobj, key, value) os.str(""); os << value; xmlobj->addAttr(key, os.str());
        SET_ATTR(obj, "id",instance->_id);
        SET_ATTR(obj, "method_id", instance->_frame.method_id);
        SET_ATTR(obj, "method_version", instance->_frame.method_version);
        SET_ATTR(obj, "binary_addr", instance->_frame.binary_addr);
        SET_ATTR(obj, "numa_node", instance->_frame.numa_node);
        SET_ATTR(obj, "bci", instance->_frame.bci);
        if (instance->_parent){
            SET_ATTR(obj, "parent_id", instance->_parent->_id);
        }
        else {
            SET_ATTR(obj, "parent_id", -1);
        }

        //add metrics as child
        if (instance->_metrics) {
            XMLObj *child = xml::createXMLObj(instance->_metrics);
            if (child != nullptr) {
                obj->addChild(0, child);
            } else {
                delete obj;
                obj = nullptr;
            }
        }
#undef SET_ATTR

        return obj;
    }
}

NewContextTree::NewContextTree() {
    _root = new(std::nothrow) NewContext(0);
    assert(_root);
    _context_map[0] = _root;
    _id_counter = 1;
}

NewContextTree::~NewContextTree() {
    for (auto &it : _context_map) {
        delete it.second;
    }
    _context_map.clear();
}

NewContext *NewContextTree::addContext(uint32_t parent_id, const NewContextFrame &frame) {
    auto it = _context_map.find(parent_id);
    assert(it != _context_map.end());
    NewContext *parent = it->second;
    return addContext(parent, frame);
}

NewContext *NewContextTree::addContext(NewContext *parent, const NewContextFrame &frame) {
    assert(parent);
    //create a new context
    uint32_t ctxt_id = _id_counter;
    NewContext * new_ctxt = new(std::nothrow) NewContext(ctxt_id);
    assert(new_ctxt);
    (*new_ctxt) = frame;
    //see whether parent already has the children
    NewContext *find_child = parent->findChild(new_ctxt);
    // the parent already has this child context, no further action
    if (find_child) {
        delete new_ctxt;
        new_ctxt = nullptr;
        return find_child;
    }

    // add the new node under the parent
    parent->addChild(new_ctxt);
    // add the new context to the tree
    _context_map[ctxt_id] = new_ctxt;
    _id_counter++;

    return new_ctxt;
}
