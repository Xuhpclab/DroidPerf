#!/usr/bin/python3

import os
import sys
import drcctprof_data_builder as ddb
# import sys
# import json

g_file_map = dict()

# output_root = sys.argv[1]

def get_file_path(file_name, class_name):
    return file_name
    package_name = class_name.rsplit(".", 1)[0]
    if package_name + ":" + file_name in g_file_map:
        return g_file_map[package_name + ":" + file_name]
    else:
        return file_name

def update_sourcefile_map(file_name, filepath, type):
    with open(filepath,'r') as file:
        content = file.readlines()
        for line in content:
            if line.startswith('package '):
                package_name = ""
                if type == "java":
                    package_name = line[8:-2]
                else :
                    package_name = line[8:-1]
                # print(package_name + ":" + file_name + " " + filepath)
                g_file_map[package_name + ":" + file_name] = filepath
                break

def init_sourcefile_map(path):
    filelist = os.listdir(path)
    for filename in filelist:
        filepath = os.path.join(path, filename)
        if os.path.isdir(filepath):
            init_sourcefile_map(filepath)
        else:
            if os.path.splitext(filename)[-1][1:] == "java" or os.path.splitext(filename)[-1][1:] == "scala" :
                update_sourcefile_map(filename, filepath, os.path.splitext(filename)[-1][1:])



context_root = {
    "id": 0,
    "pid": 0,
    "mid": -1,
    "lineNo":0,
    "children": {}
}

context_array = [context_root]

def getContextId(parent, frames):
    curContext = {}
    frame = frames[0]
    if frame[0] + ":" + frame[1] in parent["children"]:
        curContext = parent["children"][frame[0] + ":" + frame[1]]
    else:
        curContext = {
            "id": len(context_array),
            "pid": parent["id"],
            "mid": int(frame[0]),
            "lineNo":int(frame[1]),
            "children": {}
        }
        context_array.append(curContext)
        parent["children"][frame[0] + ":" + frame[1]] = curContext
    if len(frames) == 1:
        return curContext["id"]
    else:
        return getContextId(curContext, frames[1:])

if __name__ == "__main__":

    # init_sourcefile_map(output_root)

    dirname = "Documents"

    dirname_full_path = os.path.abspath(dirname)

    threadProfileMap = {}
    filelist=os.listdir(dirname_full_path)

    for f in filelist:
        temp = f.split(".")[0].split("-")
        if temp[2] not in threadProfileMap:
            threadProfileMap[temp[2]] = {};
        threadProfileMap[temp[2]][temp[0]] = f;
    # print(threadProfileMap)

    trace_thread = []
    alloc_thread = []
    cachemiss_thread = []
    for key in threadProfileMap.keys():
        for content in threadProfileMap[key]:
          if content == "trace":
            trace_thread.append(key)
          if content == "alloc":
            alloc_thread.append(key)
          if content == "cachemiss":
            cachemiss_thread.append(key)

    builder = ddb.Builder()
    builder.addMetricType(1, "cpu cycle", "cpu cycle")

    for key in cachemiss_thread:
        i = 0
        for content in threadProfileMap[key]:
          i = i + 1
        if i < 2:
          continue;
        method_file = threadProfileMap[key]["method"]
        cachemiss_file = threadProfileMap[key]["cachemiss"]

        mf = open(dirname_full_path+"/"+method_file);
        method_map = {-1:{
            "name": "ROOT",
            "file_path": " "
        }, 0:{
            "name": "<NULL>",
            "file_path": " "
        } }
        method_id_list = []
        for line in mf:
            temp = line.split(" ")
            if len(temp) < 3:
              continue
            method_map[int(temp[0])] = {
                "name": get_file_path(temp[2], "").rstrip() + "." + temp[1],
                "file_path": get_file_path(temp[2], "")
            }
            method_id_list.append(int(temp[0]))
        # print(method_id_list)
        tf = open(dirname_full_path+"/"+cachemiss_file);
        trace_map = []
        for line in tf:
            trace = {}
            if line.find(" |") == -1:
                continue;
            temp = line.split(" |")
            trace['t'] = []
            trace['m'] = int(temp[1])
            temp_trace = temp[0].split(" ")
            for frame in temp_trace:
                # if frame.split(":")[1] not in method_id_list:
                    # print(frame.split(":")[1]) 
                trace['t'].append([frame.split(":")[1], frame.split(":")[0]])
            # print(trace['t'])
            
            metricMsgList = [ddb.MetricMsg(0, trace['m'], "")]
            contextMsgList = []
            # contextMsgList.append(ddb.ContextMsg(1, "", "ROOT", "ROOT", 0, 0))
            for i in range(len(trace['t'])):
                # print(trace['t'][0:i+1])
                ctxt_id = getContextId(context_root, trace['t'][0:i + 1])
                ctxt = context_array[ctxt_id]
                # print(ctxt_id)
                # print(ctxt["mid"])
                if ctxt["mid"] in method_map:
                    lst = method_map[ctxt["mid"]]["file_path"]
                    index = lst.rfind(']')
                    new = lst[index+1:]

                    lst2 = method_map[ctxt["mid"]]['name']
                    index2 = lst2.rfind(']')
                    new2 = lst2[index2+1:]
                    contextMsgList.append(ddb.ContextMsg(ctxt_id, new, new2, new2, ctxt["lineNo"], ctxt["lineNo"]))
                    # contextMsgList.append(ddb.ContextMsg(ctxt_id, method_map[ctxt["mid"]]["file_path"], method_map[ctxt["mid"]]['name'], method_map[ctxt["mid"]]['name'], ctxt["lineNo"], ctxt["lineNo"]))
            # print(len(contextMsgList))
            builder.addSample(contextMsgList, metricMsgList) 
    builder.generateProfile("./Results/cachemiss.drcctprof")


    builder2 = ddb.Builder()
    builder2.addMetricType(1, "cpu cycle", "cpu cycle")

    for key in trace_thread:
        i = 0
        for content in threadProfileMap[key]:
          i = i + 1
        if i < 2:
          continue;
        method_file = threadProfileMap[key]["method"]
        trace_file = threadProfileMap[key]["trace"]

        mf = open(dirname_full_path+"/"+method_file);
        method_map = {-1:{
            "name": "ROOT",
            "file_path": " "
        }, 0:{
            "name": "<NULL>",
            "file_path": " "
        } }
        method_id_list = []
        for line in mf:
            temp = line.split(" ")
            if len(temp) < 3:
              continue
            method_map[int(temp[0])] = {
                "name": get_file_path(temp[2], "").rstrip() + "." + temp[1],
                "file_path": get_file_path(temp[2], "")
            }
            method_id_list.append(int(temp[0]))
        # print(method_id_list)
        tf = open(dirname_full_path+"/"+trace_file);
        trace_map = []
        for line in tf:
            trace = {}
            if line.find(" |") == -1:
                continue;
            temp = line.split(" |")
            trace['t'] = []
            trace['m'] = int(temp[1])
            temp_trace = temp[0].split(" ")
            for frame in temp_trace:
                # if frame.split(":")[1] not in method_id_list:
                    # print(frame.split(":")[1]) 
                trace['t'].append([frame.split(":")[1], frame.split(":")[0]])
            # print(trace['t'])
            
            metricMsgList = [ddb.MetricMsg(0, trace['m'], "")]
            contextMsgList = []
            # contextMsgList.append(ddb.ContextMsg(1, "", "ROOT", "ROOT", 0, 0))
            for i in range(len(trace['t'])):
                # print(trace['t'][0:i+1])
                ctxt_id = getContextId(context_root, trace['t'][0:i + 1])
                ctxt = context_array[ctxt_id]
                # print(ctxt_id)
                # print(ctxt["mid"])
                if ctxt["mid"] in method_map:

                    if i == len(trace['t']) - 1: #leaf node
                      contextMsgList.append(ddb.ContextMsg(ctxt_id, method_map[ctxt["mid"]]["file_path"], method_map[ctxt["mid"]]['name'], method_map[ctxt["mid"]]['name'], ctxt["lineNo"], ctxt["lineNo"]))
          
                    else: #non-leaf node, i from 0 to len(trace['t']) - 2
                      lst = method_map[ctxt["mid"]]["file_path"]
                      index = lst.rfind(']')
                      new = lst[index+1:]

                      lst2 = method_map[ctxt["mid"]]['name']
                      index2 = lst2.rfind(']')
                      new2 = lst2[index2+1:]
                      contextMsgList.append(ddb.ContextMsg(ctxt_id, new, new2, new2, ctxt["lineNo"], ctxt["lineNo"]))
                      # contextMsgList.append(ddb.ContextMsg(ctxt_id, method_map[ctxt["mid"]]["file_path"], method_map[ctxt["mid"]]['name'], method_map[ctxt["mid"]]['name'], ctxt["lineNo"], ctxt["lineNo"]))
                      
            # print(len(contextMsgList))
            builder2.addSample(contextMsgList, metricMsgList) 
    builder2.generateProfile("./Results/access.drcctprof")  


    builder3 = ddb.Builder()
    builder3.addMetricType(1, "cpu cycle", "cpu cycle")

    for key in alloc_thread:
        i = 0
        for content in threadProfileMap[key]:
          i = i + 1
        if i < 2:
          continue;
        method_file = threadProfileMap[key]["method"]
        trace_file = threadProfileMap[key]["alloc"]

        mf = open(dirname_full_path+"/"+method_file);
        method_map = {-1:{
            "name": "ROOT",
            "file_path": " "
        }, 0:{
            "name": "<NULL>",
            "file_path": " "
        } }
        for line in mf:
            temp = line.split(" ")
            if len(temp) < 3:
              continue
            method_map[int(temp[0])] = {
                "name": get_file_path(temp[2], "").rstrip() + "." + temp[1],
                "file_path": get_file_path(temp[2], "")
            }
        tf = open(dirname_full_path+"/"+trace_file);
        trace_map = []
        for line in tf:
            trace = {}
            if line.find(" |") == -1:
              continue
            temp = line.split(" |")
            trace['t'] = []
            trace['m'] = int(temp[1])
            temp_trace = temp[0].split(" ")
            for frame in temp_trace:
                trace['t'].append([frame.split(":")[1], frame.split(":")[0]])
            
            metricMsgList = [ddb.MetricMsg(0, trace['m'], "")]
            contextMsgList = []
            # contextMsgList.append(ddb.ContextMsg(1, "", "ROOT", "ROOT", 0, 0))
            key_not_exist = 0
            for i in range(len(trace['t'])):
                # print(trace['t'][0:i+1])
                ctxt_id = getContextId(context_root, trace['t'][0:i + 1])
                ctxt = context_array[ctxt_id]
                # print(ctxt_id)
                if ctxt["mid"] not in method_map:
                  key_not_exist = 1
                  break;

                lst = method_map[ctxt["mid"]]["file_path"]
                index = lst.rfind(']')
                new = lst[index+1:]

                lst2 = method_map[ctxt["mid"]]['name']
                index2 = lst2.rfind(']')
                new2 = lst2[index2+1:]
                contextMsgList.append(ddb.ContextMsg(ctxt_id, new, new2, new2, ctxt["lineNo"], ctxt["lineNo"]))
                # contextMsgList.append(ddb.ContextMsg(ctxt_id, method_map[ctxt["mid"]]["file_path"], method_map[ctxt["mid"]]['name'], method_map[ctxt["mid"]]['name'], ctxt["lineNo"], ctxt["lineNo"]))
            # print(len(contextMsgList))
            if key_not_exist == 0:
              builder3.addSample(contextMsgList, metricMsgList) 
    builder3.generateProfile("./Results/alloc.drcctprof")

     

    
    # gf = ht.GraphFrame.from_hpctoolkit(dirname)
    # # print(gf.dataframe.to_dict("records"))
    # for row in gf.dataframe.itertuples():
    #     #print (row)
    #     node = row[0][0]
    #     # Only consider the leaf nodes
    #     if node.children == []:
    #         #print(row[2])
    #         metricMsgList = []
    #         metricMsgList.append(ddb.MetricMsg(0, row[2]*1000000, ""))
    #         contextMsgList = []
    #         for frame in node.path():
    #             # node + rank for index
    #             if len(row[0]) == 2:
    #                 contextMsgDic = gf.dataframe.loc[(frame, 0, )].to_dict()
    #             # node + rank + tid for index
    #             else:
    #                 contextMsgDic = gf.dataframe.loc[(frame, 0, 0)].to_dict()
    #             # print(context.to_dict())
    #             file = contextMsgDic['file']
    #             if file[0] == '.':
    #                 file = dirname_full_path+file[1:]
    #             contextMsgList.append(ddb.ContextMsg(contextMsgDic['nid'], file, contextMsgDic['name'], contextMsgDic['name'], 0, contextMsgDic['line']))
    #         builder.addSample(contextMsgList, metricMsgList)
    # builder.generateProfile("test.drcctprof")