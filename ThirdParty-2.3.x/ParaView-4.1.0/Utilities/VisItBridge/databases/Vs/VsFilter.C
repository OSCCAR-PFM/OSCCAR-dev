

// For whatever reason hdf5 1.8.4 on Ubunut defines the 1.6 API as default...
// Force using the 1.8 API in this situation. These must be defined before
// including hdf5.h

#define H5Dopen_vers 2
#define H5Gopen_vers 2
#define H5Oopen_vers 2
#define H5Aiterate_vers 2

#include <hdf5.h>
#include <visit-hdf5.h>
#if HDF5_VERSION_GE(1,8,1)
/**
 *
 * @file        VsFilter.cpp
 *
 * @brief       Implementation for inspecting an HDF5 file
 *
 * @version $Id: VsFilter.C 496 2009-08-05 22:57:22Z mdurant $
 *
 * Copyright &copy; 2007-2008, Tech-X Corporation
 * See LICENSE file for conditions of use.
 *
 */

// vsh5
#include <VsFilter.h>
#include <VsSchema.h>
#include <VsUtils.h>
#include <hdf5.h>
#include <stdlib.h>

struct RECURSION_DATA {
  VsH5Meta* meta;
  std::ostream* debugStream;
  int depth;
  std::string path;
};

VsFilter::VsFilter(std::ostream& dbgstrm) : debugStrmRef(dbgstrm) {
}

VsFilter::VsFilter(hid_t fId, std::ostream& dbgstrm) :
debugStrmRef(dbgstrm), fileId(fId) {
  makeH5Meta();
}

void VsFilter::makeH5Meta() {
  h5meta.ptr = 0;
  // Create the list of vs datasets and groups
  // corresponding to variables, variables with mesh and meshes.
  RECURSION_DATA data;
  data.debugStream = &debugStrmRef;
  data.meta = &h5meta;
  data.depth = 0;
  data.path = "";

  H5Literate(fileId, H5_INDEX_NAME, H5_ITER_INC, 0, visitLinks, &data);
}

herr_t VsFilter::visitLinks(hid_t locId, const char* name,
    const H5L_info_t *linfo, void* opdata) {
  RECURSION_DATA* data = static_cast< RECURSION_DATA* >(opdata);
  VsH5Meta* metaPtr = data->meta;
  std::ostream& osRef = *(data->debugStream);

  //   for(int i=0;i<data->depth; ++i ) std::cerr << "    ";
  //   std::cerr << "Link " << name << "  " << locId << std::endl;

  //the fully qualified name of this object
  std::string fullName = makeCanonicalName(data->path, name);//data->path + "/" + name;

  switch (linfo->type) {
    case H5L_TYPE_HARD: {

      H5O_info_t objinfo;

      /* Stat the object */
      if(H5Oget_info_by_name(locId, name, &objinfo, H5P_DEFAULT) < 0) {
        // DO TO - THROW AN ERROR
      }

      switch(objinfo.type)
      {
        case H5O_TYPE_GROUP:
        return visitGroup( locId, name, opdata );
        break;
        case H5O_TYPE_DATASET:
        return visitDataset( locId, name, opdata );
        break;

        default:
        osRef << "VsFilter::visitLinks: node '" << name <<
        "' has an unknown type " << objinfo.type << std::endl;
        break;
      }

      break;
    }

    case H5L_TYPE_EXTERNAL: {

      char *targbuf = (char*) malloc( linfo->u.val_size );

      if(H5Lget_val(locId, name, targbuf, linfo->u.val_size,
              H5P_DEFAULT) < 0) {

        // DO TO - THROW AN ERROR
      } else {
        const char *filename;
        const char *targname;

        if(H5Lunpack_elink_val(targbuf, linfo->u.val_size, 0,
                &filename, &targname) < 0) {
          // DO TO - THROW AN ERROR
        } else {

          osRef << "VsFilter::visitLinks: node '" << name << "' is an external link." << std::endl;

          osRef << "VsFilter::visitLinks: node '" << targname << "' the is an external target group." << std::endl;

          free(targbuf);

          // Open the linked object.
          H5O_info_t objinfo;
          hid_t obj_id = H5Oopen(locId, name, H5P_DEFAULT);
          if ( obj_id < 0 ) {
            // DO TO - THROW AN ERROR
          }

          else if ( H5Oget_info ( obj_id, &objinfo ) < 0 ) {
            // DO TO - THROW AN ERROR
          }

          else {

            H5Oclose( obj_id );

            switch(objinfo.type)
            {
              case H5O_TYPE_GROUP:
              return visitGroup( locId, name, opdata );
              break;
              case H5O_TYPE_DATASET:
              return visitDataset( locId, name, opdata );
              break;

              default:
              osRef << "VsFilter::visitLinks: node '" << name <<
              "' has an unknown type " << objinfo.type << std::endl;
              break;
            }
          }
        }
      }
    }
    break;

    default:
    osRef << "VsFilter::visitLinks: node '" << name <<
    "' has an unknown object type " << linfo->type << std::endl;
    break;
  }

  //   for( int i=0;i<data->depth; ++i ) std::cerr << "    ";
  //   std::cerr << "Out Link " << name << std::endl;

  return 0;
}

herr_t VsFilter::visitGroup(hid_t locId, const char* name, void* opdata) {
  RECURSION_DATA* data = static_cast< RECURSION_DATA* >(opdata);
  VsH5Meta* metaPtr = data->meta;
  std::ostream& osRef = *(data->debugStream);

  //the fully qualified name of this object
  std::string fullName = makeCanonicalName(data->path, name);//data->path + "/" + name;

  //   for( int i=0;i<data->depth; ++i ) std::cerr << "    ";
  //   std::cerr << "Group " << name << "  " << locId << std::endl;

  osRef << "VsFilter::visitGroup: node '" << name
  << "' is a group." << std::endl;

  hid_t groupId = H5Gopen(locId, name, H5P_DEFAULT);

  if ( groupId < 0 ) {
    // DO TO - THROW AN ERROR
  }

  osRef << "VsFilter::visitGroup: node '" << name <<
  "' opened." << std::endl;

  // Metadata for this group
  VsGMeta* gm = new VsGMeta();
  gm->name = name;
  gm->depth = data->depth + 1;
  gm->path = data->path;
  gm->iid = groupId;
  // Collect attributes of the group
  std::pair<VsIMeta*, std::ostream*> gpd(gm, &osRef);
  H5Aiterate(groupId, H5_INDEX_NAME, H5_ITER_INC, NULL, visitAttrib, &gpd);

  // Check is gm's isMesh is true and add to meshes
  // Check if the group is a mesh and register.  Move the ptr to the mesh
  if (gm->isMesh) {
    osRef << "VsFilter::visitGroup: node '" << name << "' is a mesh."
    << std::endl;
    std::pair<std::string, VsGMeta*> el(fullName, gm);
    metaPtr->gMeshes.insert(el);
    metaPtr->ptr = gm;
  }
  else if (gm->isVsVars) {
    osRef << "VsFilter::visitGroup: node '" << name << "' is vsVars." <<
    std::endl;
    std::pair<std::string, VsGMeta*> el(fullName, gm);
    metaPtr->vsVars.insert(el);
    metaPtr->ptr = gm;
  }
  else {
    // If group is not a mesh - delete it - needs to be changed to work with groups
    // which are variables
    osRef << "VsFilter::visitGroup: node '" << name << "' is not a mesh."
    << "  Deleting." << std::endl;
    delete gm;
    gm = 0;
    // When deleting the meta data the group is closed so reopen.
    // THIS PRACTICE OF HAVING ONE FUNCTION OPEN A GROUP BUT THEN
    // HAVE A STRUCTURE STORING IT AND THEN CLOSE IT ON
    // DESTRUCTION IS NOT A GOOD PRACTIC
    groupId = H5Gopen(locId, name, H5P_DEFAULT);
  }

  // recurse to examine child groups
  RECURSION_DATA nextLevelData;
  nextLevelData.debugStream = data->debugStream;
  nextLevelData.depth = data->depth + 1;
  nextLevelData.path = fullName;
  nextLevelData.meta = data->meta;

  osRef <<"VsFilter::visitGroup(...): Recursing with path " <<fullName <<" and depth " <<nextLevelData.depth <<"." <<std::endl;

  H5Literate(groupId, H5_INDEX_NAME, H5_ITER_INC, NULL, visitLinks, &nextLevelData);
  // Just a group so close it.
  if( gm == 0 )
  H5Gclose( groupId );

  osRef <<"VsFilter::visitGroup: Returned from recursion" <<std::endl;

  return 0;
}

herr_t VsFilter::visitDataset(hid_t locId, const char* name, void* opdata) {
  RECURSION_DATA* data = static_cast< RECURSION_DATA* >(opdata);
  VsH5Meta* metaPtr = data->meta;
  std::ostream& osRef = *(data->debugStream);

  //the fully qualified name of this object
  std::string fullName = makeCanonicalName(data->path, name);//data->path + "/" + name;

  //   for( int i=0;i<data->depth; ++i ) std::cerr << "    ";
  //   std::cerr << "Group " << name << "  " << locId << std::endl;

  osRef << "VsFilter::visitDataset: node '" << name <<
  "' is a dataset." << std::endl;

  hid_t datasetId = H5Dopen(locId, name, H5P_DEFAULT);

  if ( datasetId < 0 ) {
    // DO TO - THROW AN ERROR
  }

  osRef << "VsFilter::visitDataset: node '" << name <<
  "' opened." << std::endl;

  // Metadata for this dataset
  VsDMeta* vm = new VsDMeta();
  std::string sname = name;
  vm->name = sname;
  vm->path = data->path;
  vm->depth = data->depth + 1;
  vm->type = H5Tget_native_type(H5Dget_type(datasetId), H5T_DIR_DEFAULT);
  vm->iid = datasetId;
  getDims(datasetId, true, vm->dims);
  // Collect attributes of this dataset
  std::pair<VsIMeta*, std::ostream*> gpd(vm, &osRef);
  H5Aiterate(datasetId, H5_INDEX_NAME, H5_ITER_INC, NULL, visitAttrib,
      &gpd);

  //all datasets get registered in a general list
  std::pair<std::string, VsDMeta*> el(fullName, vm);
  metaPtr->datasets.insert(el);

  // Test isVariable etc and add register
  if (vm->isVariable) {
    std::pair<std::string, VsDMeta*> el2(fullName, vm);
    metaPtr->vars.insert(el2);
    osRef <<"VsFilter::visitDataset: node '" <<name <<
    "' is a Variable." <<std::endl;
  }
  else if (vm->isVariableWithMesh) {
    std::pair<std::string, VsDMeta*> el2(fullName, vm);
    metaPtr->varsWithMesh.insert(el2);
    osRef <<"VsFilter::visitDataset: node '" <<name <<
    "' is a variable with mesh." <<std::endl;
  }
  else if (vm->isMesh) {
    std::pair<std::string, VsDMeta*> el2(fullName, vm);
    metaPtr->dMeshes.insert(el2);
    osRef <<"VsFilter::visitDataset: node '" <<name <<
    "' is a dataset mesh." <<std::endl;
  } else {
    //This dataset is neither a var, a mesh, or a varWithMesh
    //If it exists inside a Mesh group, then it might still be used
    // (e.g. as a vsPoints dataset)
    //However, if it's not in a mesh group, it's not used
    // so it gets deleted
    if (metaPtr->ptr) {
      //we have a parent - is it a Mesh group?
      VsGMeta* parent = static_cast<VsGMeta*>(metaPtr->ptr);
      if (parent && (parent->isMesh)) {
        parent->datasets.push_back(vm);
      } else {
        osRef <<"VsFilter::visitDataset: node '" <<name <<"' is not recognized as a Vs object.  Remembering for later." <<std::endl;
        metaPtr->orphanDatasets.push_back(vm);
        //        delete vm;
      }
    } else {
      osRef <<"VsFilter::visitDataset: node '" <<name <<"' is not recognized as a Vs object.  Remembering for later." <<std::endl;
      metaPtr->orphanDatasets.push_back(vm);
      //      delete vm;
    } 
  }

  osRef <<"VsFilter::visitDataset: Returning from recursion" <<std::endl;

  return 0;
}

herr_t VsFilter::visitAttrib(hid_t dId, const char* name,
    const H5A_info_t* ai, void* opdata) {

  std::pair<VsIMeta*, std::ostream*>* gpdPtr =
  static_cast<std::pair<VsIMeta*, std::ostream*>*>(opdata);
  // VsIMeta* iMetaPtr = static_cast<VsIMeta*>(opdata);
  VsIMeta* iMetaPtr = gpdPtr->first;
  std::ostream& osRef = *gpdPtr->second;

  osRef << "VsFilter::visitAttrib(...): getting attribute '" <<
  name << "'." << std::endl;

  hid_t attId = H5Aopen_name(dId, name);
  // Metadata for this attribute
  VsAMeta* am = new VsAMeta();
  am->type = H5Tget_native_type(H5Aget_type(attId), H5T_DIR_DEFAULT);
  am->aid = attId;
  am->name = name;
  // 0 shows that it is attribute
  getDims(am->aid, false, am->dims);
  // add attrib metadata
  am->depth = iMetaPtr->depth+1;
  iMetaPtr->attribs.push_back(am);
  // check for type attribute
  std::string aname = name;
  if (aname.compare(VsSchema::typeAtt) == 0) {
    std::string attValue;
    herr_t err = getAttributeHelper(attId, &attValue, 0, 0);
    if (err == 0) {
      osRef << "VsFilter::visitAttrib(...): attribute '" << name <<
      "' is '" << attValue << "'." << std::endl;
      // See if this dataset or group is a mesh
      if (attValue.compare(VsSchema::meshKey) == 0) {
        iMetaPtr->isMesh = true;
      }
      // See if this group is derived vars
      if ( (iMetaPtr->isGroup) && (attValue.compare(VsSchema::vsVarsKey) == 0)) {
        (static_cast<VsGMeta*>(iMetaPtr))->isVsVars = true;
      }
      if (!iMetaPtr->isGroup) {
        // See if this dataset is a variable or variableWithMesh
        if(attValue.compare(VsSchema::varKey) == 0) {
          iMetaPtr->isVariable = true;
        }
        if(attValue.compare(VsSchema::varWithMeshKey) == 0) {
          iMetaPtr->isVariableWithMesh = true;
        }
      }
    }
    else {
      osRef << "VsFilter::visitAttrib(...): error getting attribute '" <<
      name << "'.  Is this of type H5T_STR_NULLTERM and a SCALAR?" << std::endl;
    }
  }
  else {
    osRef << "VsFilter::visitAttrib(...): attribute '" << name <<
    "' is not '" << VsSchema::typeAtt << "'." << std::endl;
  }
  return 0;
}

void VsFilter::write() const {
  h5meta.write(debugStrmRef);
}

void VsFilter::setFile(hid_t fId) {
  fileId = fId;
  h5meta.clear();
  makeH5Meta();
}
#endif
