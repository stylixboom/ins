/*
 * homography.cpp
 *
 *  Created on: January 14, 2013
 *      Author: Siriwat Kasamwattanarote
 */

#include "homography.h"

using namespace std;
using namespace alphautils;

namespace ins
{

homography::homography(void)
{
	TempDir = "l";
	struct stat statTmpDir;
	if(stat(TempDir.c_str(), &statTmpDir) != 0)
		mkdir(TempDir.c_str(), 0777);

	MatchImgDir = TempDir + "/match";
	struct stat statMatchDir;
	if(stat(MatchImgDir.c_str(), &statMatchDir) != 0)
		mkdir(MatchImgDir.c_str(), 0777);
}

homography::~homography(void)
{
	Reset();
}

void homography::Process(const string& queryfile, const string& frameroot, const vector<string>& framedir, const vector<string>& framename, const vector< pair<size_t, float> >& rank, const size_t maxLimit)
{
	lookupResult.clear();

	cout << "Matching..";
	cout.flush();
	timespec matchTime = CurrentPreciseTime();

	// Matching
	size_t rankLimSize = maxLimit;
	if(rank.size() < rankLimSize)
		rankLimSize = rank.size();
	// Prepare tasks
	string taskFileName = TempDir + "/homotasklist.hmg";
	ofstream TaskFile(taskFileName.c_str());
	if (TaskFile.is_open())
	{
		// write HoMoflaG
		TaskFile << "hmg" << endl;
		// write query
		TaskFile << queryfile << endl;
		// write frames
		for(size_t rIdx = 0; rIdx < rankLimSize; rIdx++)
		{
			size_t index = rank[rIdx].first;
			stringstream framefile;
			framefile << frameroot << "/" << framedir[index] << "/" << framename[index];
			TaskFile << rIdx << endl; // Result relative index
			TaskFile << framefile.str() << endl;
		}
		TaskFile.close();
	}
	// Call task manager
	stringstream cmd;
	cmd << "./HomoTaskManager " << taskFileName << " " << rankLimSize << COUT2NULL;
	cout << exec(cmd.str());

	// Waiting histXct result
	struct stat statHistXct;
	while(stat((taskFileName + ".ret").c_str(), &statHistXct) != 0)
		usleep(250000);

	// Post read results
	// read taskFileName + ".ret"
	ifstream RetFile((taskFileName + ".ret").c_str());
	if (RetFile)
	{
		int count = 0;
		while (RetFile.good())
		{
			count++;
			//index
			string index;
			getline(RetFile, index);
			if(index == "") // end of file with new line was read
				break;
			//score
			string score;
			getline(RetFile, score);
			lookupResult.push_back(pair<size_t, int>(strtoull(index.c_str(), NULL, 0), atoi(score.c_str()))); // push_back result lookup with value (not VID with value)
		}
		RetFile.close();
	}

	// After read!, Delete it!
	remove((taskFileName + ".ret").c_str());

	//lookupResult.push_back(pair<size_t, int>(rIdx, Matching(queryfile, framefile.str()))); // push_back result lookup with value (not VID with value)
	//cout << "Matching done: " << rIdx << " | " << framename[index] << " | " << lookupResult[rIdx].second << endl;

	double MatchTimeUsage = TimeElapse(matchTime);
	cout << "completed!";
	cout << " (" <<  MatchTimeUsage << " s)" << endl;

	cout << "Sorting..";
	cout.flush();
	timespec sortTime = CurrentPreciseTime();

	// Sorting
	//cout << endl << "Before: " << lookupResult[0].first << " " << lookupResult[1].first << " " << lookupResult[2].first << " " << lookupResult[3].first << " " << lookupResult[4].first << endl;
	//cout << "Value: " << lookupResult[0].second << " " << lookupResult[1].second << " " << lookupResult[2].second << " " << lookupResult[3].second << " " << lookupResult[4].second << endl;
	sort(lookupResult.begin(), lookupResult.end(), compare_pair_second<>());
	//cout << "After: " << lookupResult[0].first << " " << lookupResult[1].first << " " << lookupResult[2].first << " " << lookupResult[3].first << " " << lookupResult[4].first << endl;
	//cout << "Value: " << lookupResult[0].second << " " << lookupResult[1].second << " " << lookupResult[2].second << " " << lookupResult[3].second << " " << lookupResult[4].second << endl;

	double SortTimeUsage = TimeElapse(sortTime);
	cout << "completed!";
	cout << " (" <<  SortTimeUsage << " s)" << endl;
}

void homography::GetReRanked(vector< pair <size_t, int> >& result)
{
	result.clear();

	vector< pair <size_t, int> >::iterator itLookup;

	for(itLookup = lookupResult.begin(); itLookup != lookupResult.end(); itLookup++)
		result.push_back(*itLookup);
}

void homography::Reset(void)
{
	lookupResult.clear();
	vector< vector < DMatch > >::iterator itRaw;
	for(itRaw = rawResult.begin(); itRaw != rawResult.end(); itRaw++)
		(*itRaw).clear();
	rawResult.clear();
}

}


