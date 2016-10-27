// Copyright 2011 The University of Michigan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors - Jie Yu (jieyu@umich.edu)

// File: core/static_info.cc - Static information for program binary or
// library binaries.

#include "core/static_info.h"

#include <fstream>
#include <sstream>

#include <unistd.h>

Inst *Image::Find(address_t offset) {
  InstAddrMap::iterator found = inst_offset_map_.find(offset);
  if (found == inst_offset_map_.end())
    return NULL;
  else
    return found->second;
}

bool Image::IsCommonLib() {
  if ((name().find("libc") != std::string::npos) ||
      (name().find("libpthread") != std::string::npos) ||
      (name().find("ld-") != std::string::npos) ||
      //(name().find("libstdc++") != std::string::npos) ||
      (name().find("libgcc_s") != std::string::npos) ||
      (name().find("libm") != std::string::npos) ||
      (name().find("libnsl") != std::string::npos) ||
      (name().find("librt") != std::string::npos) ||
      (name().find("libdl") != std::string::npos) ||
      (name().find("libz") != std::string::npos) ||
      (name().find("libcrypt") != std::string::npos) ||
      (name().find("libdb") != std::string::npos) ||
      (name().find("libexpat") != std::string::npos) ||
      (name().find("libbz2") != std::string::npos))
    return true;
  else
    return false;
}

bool Image::IsLibc() {
  if (name().find("libc") != std::string::npos)
    return true;
  else
    return false;
}

bool Image::IsPthread() {
  if (name().find("libpthread") != std::string::npos)
    return true;
  else
    return false;
}

std::string Image::ShortName() {
  size_t found = name().find_last_of('/');
  if (found != std::string::npos)
    return name().substr(found + 1);
  else
    return name();
}

void Image::Register(Inst *inst) {
  inst_offset_map_[inst->offset()] = inst;
}

void Inst::SetDebugInfo(const std::string &file_name, int line, int column) {
  DebugInfoProto *di_proto = proto_->mutable_debug_info();
  di_proto->set_file_name(file_name);
  di_proto->set_line(line);
  di_proto->set_column(column);
}

std::string Inst::DebugInfoStr() {
  if (!HasDebugInfo()) {
    using namespace std;
    string sourceFile("??");
    int lineNumber=0;
    
    struct Mapped {
      size_t start;
      size_t end;
      size_t offset;
      string file;
    };
    map<size_t, Mapped> mapped;
    //INT pid = PIN_GetPid();
    pid_t pid = getpid();
    ifstream mapsFile(string("/proc/") += to_string(pid) += "/maps");
    string line;
    while (getline(mapsFile, line)) {
      Mapped newMapped;
      size_t dash = line.find('-');
      size_t firstSpace = line.find(' ');
      size_t firstSlash = line.find('/');
      size_t secondSpace = line.find(' ', firstSpace + 1);
      size_t thirdSpace = line.find(' ', secondSpace + 1);
      {
        stringstream ss;
        ss << std::hex << line.substr(0, dash);
        ss >> newMapped.start;
      }
      {
        stringstream ss;
        ss << std::hex << line.substr(dash + 1, firstSpace - dash);
        ss >> newMapped.end;
      }
      {
        stringstream ss;
        ss << std::hex
            << line.substr(secondSpace + 1, thirdSpace - secondSpace);
        ss >> newMapped.offset;
      }
      if (firstSlash != string::npos) {
        newMapped.file = line.substr(firstSlash);
      }
      //std::cout << "Line is: " << line << std::endl;
      //std::cout << "Adding: " << newMapped.start << " " << newMapped.end << " " << newMapped.file << std::endl;
      mapped[newMapped.start] = newMapped;
    }
  
    auto it = mapped.upper_bound(pc);
    if (it != mapped.begin()) {
      --it;
      const Mapped& mappedFile = it->second;
      if (mappedFile.start <= pc && mappedFile.end >= pc
          && !mappedFile.file.empty()) {
        stringstream ss;
        ss << hex << "addr2line -e " << mappedFile.file << " " << pc;
        const int max_size = 2000;
        char sourceInfo[max_size];
        sourceInfo[0] = 0;
        {
          FILE* stdout = popen(ss.str().c_str(), "r");
          fgets(sourceInfo, max_size, stdout);
          pclose(stdout);
        }
        string sourceInfoStr(sourceInfo);
        size_t colonPos = sourceInfoStr.find(':');
        if (colonPos != string::npos) {
          sourceFile = sourceInfoStr.substr(0, colonPos);
          string sourceLine = sourceInfoStr.substr(colonPos + 1);
          if (!sourceLine.empty() && sourceLine[0] != '?') {
            lineNumber = stoi(sourceLine);
          }
        }
      }
    }
    SetDebugInfo(sourceFile, lineNumber, 0);
    
  } 
  
  const DebugInfoProto &di_proto = proto_->debug_info();
  //size_t found = di_proto.file_name().find_last_of('/');
  std::stringstream ss;
  //if (found != std::string::npos)
  //  ss << di_proto.file_name().substr(found + 1);
  //else
    ss << di_proto.file_name();
  ss << ":" << std::dec << di_proto.line();
  return ss.str();
}

std::string Inst::ToString() {
  std::stringstream ss;
  ss << std::hex << id() << " " << image_->ToString() << " 0x" << offset();
  if (HasDebugInfo())
    ss << " (" << DebugInfoStr() << ")";
  return ss.str();
}

StaticInfo::StaticInfo(Mutex *lock)
    : lock_(lock),
      curr_image_id_(0),
      curr_inst_id_(0) {
  // empty
}

Image *StaticInfo::CreateImage(const std::string &name) {
  ImageProto *image_proto = proto_.add_image();
  image_id_type image_id = GetNextImageID();
  image_proto->set_id(image_id);
  image_proto->set_name(name);
  Image *image = new Image(image_proto);
  image_map_[image_id] = image;
  return image;
}

Inst *StaticInfo::CreateInst(Image *image, address_t offset) {
  InstProto *inst_proto = proto_.add_inst();
  inst_id_type inst_id = GetNextInstID();
  inst_proto->set_id(inst_id);
  inst_proto->set_image_id(image->id());
  inst_proto->set_offset(offset);
  Inst *inst = new Inst(image, inst_proto);
  inst_map_[inst_id] = inst;
  image->Register(inst);
  return inst;
}

Image *StaticInfo::FindImage(const std::string &name2) {
  // Quick hack. Shorten name2
  // to compare with image->ShortName() below.
  std::string name;
  size_t found = name2.find_last_of('/');
  if (found != std::string::npos)
    name = name2.substr(found + 1);
  else
    name = name2;
  
  for (ImageMap::iterator it = image_map_.begin();
       it != image_map_.end(); ++it) {
    Image *image = it->second;
    if (image->ShortName().compare(name) == 0)
      return image;
  }
  return NULL;
}

Image *StaticInfo::FindImage(image_id_type id) {
  ImageMap::iterator it = image_map_.find(id);
  if (it == image_map_.end())
    return NULL;
  else
    return it->second;
}

Inst *StaticInfo::FindInst(inst_id_type id) {
  InstMap::iterator it = inst_map_.find(id);
  if (it == inst_map_.end())
    return NULL;
  else
    return it->second;
}

void StaticInfo::Load(const std::string &db_name) {
  std::fstream in(db_name.c_str(), std::ios::in | std::ios::binary);
  proto_.ParseFromIstream(&in);
  in.close();
  // setup image map
  for (int i = 0; i < proto_.image_size(); i++) {
    ImageProto *image_proto = proto_.mutable_image(i);
    Image *image = new Image(image_proto);
    image_id_type image_id = image->id();
    image_map_[image_id] = image;
    if (image_id > curr_image_id_)
      curr_image_id_ = image_id;
  }
  // setup inst map
  for (int i = 0; i < proto_.inst_size(); i++) {
    InstProto *inst_proto = proto_.mutable_inst(i);
    Image *image = FindImage(inst_proto->image_id());
    Inst *inst = new Inst(image, inst_proto);
    inst_id_type inst_id = inst->id();
    inst_map_[inst_id] = inst;
    image->Register(inst);
    if (inst_id > curr_inst_id_)
      curr_inst_id_ = inst_id;
  }
}

void StaticInfo::Save(const std::string &db_name) {
  std::fstream out(db_name.c_str(),
                   std::ios::out | std::ios::trunc | std::ios::binary);
  proto_.SerializeToOstream(&out);
  out.close();
}

