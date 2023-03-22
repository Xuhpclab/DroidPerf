#ifndef ANDROID_JXPERF_NEW_CONTEXT_H
#define ANDROID_JXPERF_NEW_CONTEXT_H

#include <stdint.h>
#include <functional>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "metrics.h"
#include "stacktraces.h"
#include "xml.h"

#define CONTEXT_TREE_ROOT_ID 0


class NewContextFrame {
public:
    NewContextFrame& operator=(const ASGCT_CallFrame &asgct_frame);

// fields
    uint64_t binary_addr = 0;
    int numa_node = 10;

    jmethodID method_id = 0;
    std::string method_name;
    uint32_t method_version = 0;

    std::string source_file;
    int32_t bci = -1; // bci is not available. We shouldn't initialize bci to 0.
    int32_t src_lineno = 0;
};


// node
class NewContext {
public:
    NewContext& operator=(const NewContextFrame &frame);

    inline void setMetrics(metrics::ContextMetrics *metrics){ _metrics = metrics;}
    inline metrics::ContextMetrics *getMetrics() {return _metrics;}
    inline const NewContextFrame &getFrame(){return _frame;}
    std::vector<NewContext *> getChildren();
    void removeChild(NewContext *ctxt);
    void addChild(NewContext *ctxt);
    NewContext *findChild(NewContext *ctxt);
    inline NewContext *getParent() {return _parent;}
    inline bool isTriggered() {return _triggered;}
    inline void setTriggered() {_triggered = true;}


private:
    NewContext(uint32_t id);
    ~NewContext();

    // the following two methods are used to distinguish children
    static std::size_t contextRefHash(const NewContext *ctxt){
        return std::hash<jint>()(ctxt->_frame.bci);
    }
    static bool contextRefEqual(const NewContext *a, const NewContext *b){
        return a->_frame.bci == b->_frame.bci &&
               a->_frame.method_id == b->_frame.method_id &&
               a->_frame.binary_addr == b->_frame.binary_addr &&
               a->_frame.numa_node == b->_frame.numa_node &&
               a->_frame.method_version == b->_frame.method_version;
    }

    uint32_t _id;
    bool _triggered = false;
    NewContextFrame _frame;
    metrics::ContextMetrics *_metrics = nullptr;

    std::unordered_set<NewContext*, std::function<std::size_t(NewContext*)>, std::function<bool(NewContext*,NewContext*)> > children;
    NewContext *_parent = nullptr;

    friend class NewContextTree;
    friend xml::XMLObj * xml::createXMLObj<NewContext>(NewContext *instance);
};


// tree
class NewContextTree {
public:
    NewContextTree();
    ~NewContextTree();

    NewContext *addContext(uint32_t parent_id, const NewContextFrame &frame);
    NewContext *addContext(NewContext *parent, const NewContextFrame &frame);

    inline NewContext *getRoot() {return _root;}

    /* iterator */
    typedef typename std::unordered_map<uint32_t,NewContext *>::iterator MAP_ITERATOR_TYPE;
    class iterator: std::iterator<std::forward_iterator_tag, NewContext *> {
    public:
        explicit iterator(MAP_ITERATOR_TYPE map_iter):_map_iterator(map_iter){}
        iterator& operator++() { _map_iterator++; return *this;}
        iterator operator++(int){_map_iterator++; return *this;}
        iterator& operator--() = delete;
        iterator operator--(int) = delete;
        bool operator==(iterator other) const {return _map_iterator == other._map_iterator;}
        bool operator!=(iterator other) const {return !(_map_iterator == other._map_iterator);}
        NewContext* operator*() const { return _map_iterator->second;}
        NewContext ** operator->() { return &(_map_iterator->second); }

    private:
        MAP_ITERATOR_TYPE _map_iterator;
    };
    iterator begin() { return iterator(_context_map.begin());}
    iterator end() { return iterator(_context_map.end());}

private:
    std::unordered_map<uint32_t, NewContext *> _context_map;
    NewContext *_root;
    uint32_t _id_counter;
};

#endif //ANDROID_JXPERF_NEW_CONTEXT_H
