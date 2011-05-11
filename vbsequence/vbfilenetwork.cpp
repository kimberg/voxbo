#include "vbfilenetwork.h"

using namespace std;
using namespace VB;

struct HasFileId
{
	std::string fileid;
	HasFileId(std::string s) : fileid(s) {}
	
	bool operator()(const JobType::File& file) { return (file.id == fileid); }
};

void FileNetwork::reset_name()
{
	if (!jobs.empty())
		curname = jobs.front()->get_jobtype()->get_file_by_id(fileid)->in_name;
	else
		curname = "";
}

void FileNetwork::insert_job(Job* job)
{
	vector<JobType::File>& files = job->get_jobtype()->files;
	if (find_if(files.begin(), files.end(), HasFileId(fileid)) != files.end())
	{
	  cerr << "  we have the right fileid" << endl;
  	typedef pair<Job*,Job*> JobPair;
  	
  	list<Job*>::reverse_iterator ji;
  	for (ji = jobs.rbegin(); ji != jobs.rend(); ++ji)
  	{
  		// look for its waitfors, from back to front
  		if (job->does_depend_on(*ji, -1))
  		{
  		  cerr << "  and there's a dependency here" << endl;
  			list<Edge>::reverse_iterator ei;
  			for (ei = edges.rbegin(); ei != edges.rend(); ++ei)
  			{
  				// if found a waitfor, look for last edge to it.
  				if ((*ei).second == *ji)
  					break;
  			}
  			if (ei.base() == edges.end()) cerr << "  we're at the end of the edges." << endl;
  			edges.insert(ei.base(), Edge(*ji,job));
  			break;
  		}
  		
  	}
  	jobs.insert(ji.base(), job);
  }
  reset_name();
}

/* check_edge will return true if the edge can pass along the file just fine 
 * without a conversion.  If a conversion is necessary, it will return false.
 */
bool FileNetwork::check_edge(Job* j1, Job* j2)
{
	return check_edge(pair<Job*,Job*>(j1,j2));
}

bool FileNetwork::check_edge(const pair<Job*,Job*>& edge)
{
	vector<JobType::File>::iterator fi1 = find_if(edge.first->get_jobtype()->files.begin(), 
	                                              edge.first->get_jobtype()->files.end(), 
	                                              HasFileId(fileid));
	vector<JobType::File>::iterator fi2 = find_if(edge.second->get_jobtype()->files.begin(), 
	                                              edge.second->get_jobtype()->files.end(), 
	                                              HasFileId(fileid));
	
  if (fi1 != edge.first->get_jobtype()->files.end() ||
      fi2 != edge.second->get_jobtype()->files.end())
  {
		JobType::File file1 = *fi1;
		JobType::File file2 = *fi2;
		
		if (file1.out_type.front() == file2.in_type.front())
			return true;
		else
			return false;
	}
//	throw string("woah there, either ") + edge.first->get_name() + " or " + 
//	  edge.second->get_name() + " doesn't have a file with id " + fileid + ".";so i 
	
	// If no edge exists between the jobs, then the edge between them is 
	// vacuously fine.
	return true;
}

/* get_edges_from/to returns the set of edges that come from or go to a given
 * job (respectively).
 */
set<FileNetwork::Edge> FileNetwork::get_edges_from(Job* j) const
{
	set<Edge> from_edges;
	vbforeach (Edge e, edges)
	{
		if (e.first == j) from_edges.insert(e);
	}
	return from_edges;
}

set<FileNetwork::Edge> FileNetwork::get_edges_to(Job* j) const
{
	set<Edge> to_edges;
	vbforeach (Edge e, edges)
	{
		if (e.second == j) to_edges.insert(e);
	}
	return to_edges;
}
