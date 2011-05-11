#ifndef __VBFILENETWORK_H__
#define __VBFILENETWORK_H__

#include <string>
#include <list>
#include <set>
#include <map>

#include "vbjob.h"

namespace VB
{

  // like a vector<tuple<Job*, Job*, File*, File*> >.  using tuple would be cool
  // but would introduce an additional library dependency (boost).  i mean, if we
  // use boost, we might as well use bgl.
  struct FileNetwork
  {
  	typedef std::pair<Job*,Job*> Edge;
  	typedef std::map<std::string, FileNetwork> Map;
  	
  	std::string fileid;
  	std::list<Edge> edges;
  	std::list<Job*> jobs;
  	std::string curname;
  	std::map<Job*,std::pair<std::string,std::string> > filenames;
  	
  	void insert_job(Job* job);
  	bool check_edge(Job*, Job*);
  	bool check_edge(const Edge& edge);
  	void reset_name();
  	
  	/* how do we get the files to correctly resolve $$ ? */
  	
  	std::set<Edge> get_edges_from(Job*) const;
  	std::set<Edge> get_edges_to(Job*) const;
  };

}

#endif // __VBFILENETWORK_H__
