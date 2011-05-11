#include "libvoxbo/vbsequence.h"
#include "libvoxbo/vbdataset.h"
#include <iostream>

using namespace std;

VBPrefs vbp;

int main(int argc, char* argv[])
{
  vbp.init();
  vbp.read_jobtypes();
  VB::Definitions::Init();
  VB::Definitions::Import_JobType_Folder();
  
  // Check for the right number of arguments.
  if (argc < 4) 
  {
    cerr << endl;
    cerr << "not enough arguments" << endl;
    cerr << argv[0] << " seq_file ds_file[:sub:node:path] out_dir" << endl;
    return 1;
  }
  
  // Construct the sequence (first argument)
  cerr << "creating the sequence..." << endl;
  ifstream seq_file;
  seq_file.open(argv[1]
  VB::Sequence* seq(new VB::Sequence(seq_file));
  seq_file.close();
  
  // Construct the dataset (second argument)
  cerr << "creating and applying the dataset..." << endl;
  VB::DataSet* ds = 0;
  string ds_arg = argv[2];
  unsigned colpos = ds_arg.find(":");
  if (colpos == string::npos) // Come here if there is no colon on the line.
  {
    ds = new VB::DataSet(ds_arg);
//    seq->set_dataset(ds);
  }
  else // Come here if there is a colon (i.e. a child dataset is specified).
  {
    string dsfile_name = ds_arg.substr(0,colpos);
    string child_name = ds_arg.substr(colpos+1);
    if (child_name != "");
    {
      ds = (new VB::DataSet(dsfile_name))->get_child(child_name);
//      if (ds) seq->set_dataset(ds);
    }
  }
  
  // Write the sequence (third argument)
  cerr << "writing the sequence to a dir..." << endl;
  seq->export_to_disk(*ds, argv[3]);
  
  cerr << "deleting the sequence..." << endl;
  delete seq;
  
  cerr << "deleting the dataset..." << endl;
  delete ds;
  
  cerr << "done!  returning successfully." << endl;
  return 0;
}
