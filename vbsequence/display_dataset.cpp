
#include "vbdataset.h"
#include "vbdatasetwidget.h"
#include "vbprefs.h"
#include <boost/foreach.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMainWindow>
#include <QBoxLayout>

#include <iostream>
#include <tr1/memory>

using namespace std;
using namespace std::tr1;

VBPrefs vbp;

void print_usage(string prg_name)
{
	cerr << "Usage: " << prg_name << " ds_file[:sub:node:path]" << endl;
	cerr << "Print the contents of the dataset to standard out." << endl;
}

int main(int argc, char* argv[])
{
  // Check for the right number of arguments.
  if (argc != 2) 
  {
    print_usage(argv[0]);
    return 1;
  }
  
  // Construct the dataset (second argument)
  cerr << "reading and building the dataset..." << endl;
  
  // Do I have to use a shared_ptr here?  No.  But I find it pleasing.
  shared_ptr<VB::DataSet> stored_pointer;
  VB::DataSet* ds=NULL;
  
  string ds_arg = argv[1];
  size_t colpos = ds_arg.find(":");
  if (colpos == string::npos) // Come here if there is no colon on the line.
  {
    stored_pointer.reset(new VB::DataSet(ds_arg));
    ds = stored_pointer.get();
  }
  else // Come here if there is a colon (i.e. a child dataset is specified).
  {
    string dsfile_name = ds_arg.substr(0,colpos);
    string child_name = ds_arg.substr(colpos+1);
    if (child_name != "")   // DYK: remove apparently accidental semicolon here
    {
    	// I cannot in good conscience leave the parent dangling.  However, if
    	// I let the parent go out of scope after this if exits, it will take the
    	// child out with it.  So I have to store the parent somewhere.
    	stored_pointer.reset(new VB::DataSet(dsfile_name));
      ds = stored_pointer->get_child(child_name);
    }
  }
  ds->import_inherited_members();
  
  // Write the sequence (third argument)
  cerr << "displaying the dataset..." << endl;
  QApplication app(argc, argv);
  QMainWindow window;
  
  QtVB::DataSetWidget tw(&window);
  tw.setDataset(*ds);
  tw.expandAll();

  window.setCentralWidget(&tw);
  window.show();
  return app.exec();
}

