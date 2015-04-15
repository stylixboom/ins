/*
 * dataset_info.cpp
 *
 *  Created on: July 9, 2012
 *      Author: Siriwat Kasamwattanarote
 */

#include "dataset_info.h"

using namespace std;
using namespace alphautils;

namespace ins
{
/// Normal dataset lookup table
/// 1:img038-00013.png

/// Commercial information lookup table
/// 1:2009_08_17_00_062013_tvtokyo~2009_08_17_00_062500_tvtokyo/2009_08_17_00_062013_tvtokyo~2009_08_17_00_062187_tvtokyo

dataset_info::dataset_info(void)
{
	_dataset_size = 0;
}

dataset_info::~dataset_info(void)
{

}

size_t dataset_info::dataset_lookup_table_init(const string& ins_root_path)
{
    vector<string> ImgParentPaths;
    vector<size_t> ImgParentsIdx;
    vector<string> ImgLists;

    ins_root = ins_root_path;

	cout << "Load dataset videos lookup table...";
	cout.flush();
	// Read parent path (dataset based path)
    ifstream InParentFile ((ins_root_path + "/dataset").c_str());
    if (InParentFile)
    {
        string read_line;
        while (!InParentFile.eof())
        {
            getline(InParentFile, read_line);
            if (read_line != "")
            {
                vector<string> split_line;
                // parent_id:parent_path
                StringExplode(read_line, ":", split_line);

                ImgParentPaths.push_back(split_line[1]);
            }
        }

        // Close file
        InParentFile.close();
    }

    // Read image filename
    ifstream InImgFile ((ins_root_path + "/dataset_file").c_str());
    if (InImgFile)
    {
        string read_line;
        while (!InImgFile.eof())
        {
            getline(InImgFile, read_line);
            if (read_line != "")
            {
                vector<string> split_line;
                // parent_id:image_name
                StringExplode(read_line, ":", split_line);

                ImgParentsIdx.push_back(atoi(split_line[0].c_str()));
                ImgLists.push_back(split_line[1]);
            }
        }

        // Close file
        InImgFile.close();
    }
	cout << "OK!" << endl;

	// Gen frame name
	for(size_t img_idx = 0; img_idx != ImgLists.size(); img_idx++)
	{
        stringstream curr_img_path;
        curr_img_path << "./dataset/" << ImgParentPaths[ImgParentsIdx[img_idx]] << "/" << ImgLists[img_idx];
        _dataset_frames_filename.push_back(curr_img_path.str());
        _dataset_size++;
	}

	return _dataset_size;
}

size_t dataset_info::dataset_cf_lookup_table_init(const string& ins_root_path)
{
    ins_root = ins_root_path;

	cout << "Load dataset videos lookup table...";
	cout.flush();
	string LookupPath;
	LookupPath = ins_root + "/lut_hesaff_rootsift_shot_full_idf_l1_flann_kdtree_1_8_800.txt";
	ifstream dataset_lookup_table_File (LookupPath.c_str());
	if (dataset_lookup_table_File)
	{
		while (dataset_lookup_table_File.good())
		{
			string lookupline;
			getline(dataset_lookup_table_File, lookupline);
			if(lookupline != "")
			{
				ShotInfo shot = ShotInfoParse(lookupline);
				_dataset_description.push_back(shot);
				_dataset_frames_dirname.push_back(shot.path);
				_dataset_size++;
			}
		}
		dataset_lookup_table_File.close();
	}
	cout << "OK!" << endl;

	cout << "Load middle frame table...";
	cout.flush();
	string dataset_middleframes_path;
	dataset_middleframes_path = ins_root + "/lut_hesaff_rootsift_shot_full_idf_l1_flann_kdtree_1_8_800_middleframe.txt";
	ifstream MiddleFrameTableFile (dataset_middleframes_path.c_str());
	if (MiddleFrameTableFile) // If file exist, load it
	{
		cout << "load...";
		cout.flush();
		while (MiddleFrameTableFile.good())
		{
			string line;
			getline(MiddleFrameTableFile, line);
			if(line != "")
			_dataset_frames_filename.push_back(trim_string_until(line, ':')); // 1:xxxx ==> xxxx
		}
		MiddleFrameTableFile.close();
	}
	else // if not exist, create it
	{
		cout << "gen...";
		cout.flush();
		vector<ShotInfo>::iterator _dataset_description_it;
		for(_dataset_description_it = _dataset_description.begin(); _dataset_description_it != _dataset_description.end(); _dataset_description_it++)
		{
			string MidFrame;
			MidFrame = GenNCheckMiddleFrameFileName(*_dataset_description_it);
			_dataset_frames_filename.push_back(MidFrame);
		}

		cout << "record...";
		cout.flush();
		ofstream CreateMiddleFrameTableFile(dataset_middleframes_path.c_str());
		if (CreateMiddleFrameTableFile.is_open())
		{
			size_t count = 0;
			vector<string>::iterator _dataset_frames_filename_it;
			for(_dataset_frames_filename_it = _dataset_frames_filename.begin(); _dataset_frames_filename_it != _dataset_frames_filename.end(); _dataset_frames_filename_it++, count++)
			CreateMiddleFrameTableFile << count << ":" << *_dataset_frames_filename_it << endl;
			CreateMiddleFrameTableFile.close();
		}
	}
	cout << "OK!" << endl;

	return _dataset_size;
}

void dataset_info::commercial_init()
{
	cout << "Load commercial data...";
	cout.flush();
	stringstream ComInfoPath;
	ComInfoPath << ins_root << "/core/db/cf.maped";
	struct stat statComInfo;
	if (stat(ComInfoPath.str().c_str(), &statComInfo) != 0)
	{
		vector<string> CfClipName;
		vector<string> CfShotName;
		vector<int> CfFreq;
		vector<string> CfIdName;
		vector<int> CfId;

		stringstream CfCfcPath;
		stringstream CfIdPath;
		stringstream CfId24Path;
		CfCfcPath << ins_root << "/core/db/cf.cfc";
		CfIdPath << ins_root << "/core/db/cf.id";
		CfId24Path << ins_root << "/core/db/cf.24.id";

		// Reading cf.cfc
		ifstream CfCfcFile (CfCfcPath.str().c_str());
		if (CfCfcFile)
		{
			cout << "reading cf.cfc...";
			cout.flush();
			while (CfCfcFile.good())
			{
				string lookupline;
				getline(CfCfcFile, lookupline);
				if(lookupline != "")
				{
					// 2009_08_17_00_001633_0000tbs 33	2009_08_17_00_001633_0000tbs 2009_08_31_21_087347_0000tbs 2009_08_21_20_080757_0000net 2009_08_21_20_081206_0000net 450
					// candidate freq	clip_start clip_end shot_start shot_end some_number

					vector<string> SpaceSubCfc;
					vector<string> TabSubCfc;

					StringExplode(lookupline, " ", SpaceSubCfc);
					CfClipName.push_back(SpaceSubCfc[0]);
					CfShotName.push_back(SpaceSubCfc[3]);
					StringExplode(SpaceSubCfc[1], "\t", TabSubCfc);
					CfFreq.push_back(atoi(TabSubCfc[0].c_str()));

					SpaceSubCfc.clear();
					TabSubCfc.clear();
				}
			}
			CfCfcFile.close();
		}

		// Reading cf.id
		ifstream CfIdFile (CfIdPath.str().c_str());
		if (CfIdFile)
		{
			cout << "reading cf.id...";
			cout.flush();
			while (CfIdFile.good())
			{
				string lookupline;
				getline(CfIdFile, lookupline);
				if(lookupline != "")
				{
					// 2009_10_26_08_075512_0000tbs 1

					vector<string> SpaceSubId;

					StringExplode(lookupline, " ", SpaceSubId);
					CfIdName.push_back(SpaceSubId[0]);
					CfId.push_back(atoi(SpaceSubId[1].c_str()));

					SpaceSubId.clear();
				}
			}
			CfIdFile.close();
		}

		// Reading cf.24.id
		ifstream CfId24File (CfId24Path.str().c_str());
		if (CfId24File)
		{
			cout << "reading cf.24.id...";
			cout.flush();
			while (CfId24File.good())
			{
				string lookupline;
				getline(CfId24File, lookupline);
				if(lookupline != "")
				{
					// 2009_10_26_08_075512_0000tbs 1

					vector<string> SpaceSubId;

					StringExplode(lookupline, " ", SpaceSubId);
					CfIdName.push_back(SpaceSubId[0]);
					CfId.push_back(atoi(SpaceSubId[1].c_str()));

					SpaceSubId.clear();
				}
			}
			CfId24File.close();
		}

		// Mapping
		ofstream ComInfoFile (ComInfoPath.str().c_str());
		if (ComInfoFile.is_open())
		{
			cout << endl << "mapping..." << endl;
			cout.flush();
			string PrevCandidateString = "";
			for(size_t indexLut = 0; indexLut < _dataset_description.size(); indexLut++)
			{
				// Make candidate name
				stringstream CandidateString;
				CandidateString << _dataset_description[indexLut].year << "_" <<
				ZeroPadNumber(_dataset_description[indexLut].month, 2) << "_" <<
				ZeroPadNumber(_dataset_description[indexLut].day, 2) << "_" <<
				ZeroPadNumber(_dataset_description[indexLut].hour, 2) << "_" <<
				ZeroPadNumber(_dataset_description[indexLut].cStart, 6) << "_" <<
				ZeroPadString(_dataset_description[indexLut].channel, 7);

				if(PrevCandidateString == CandidateString.str())
				{
					_dataset_description[indexLut].freq = _dataset_description[indexLut - 1].freq;
					_dataset_description[indexLut].cfid = _dataset_description[indexLut - 1].cfid;
					ComInfoFile << _dataset_description[indexLut].freq << " " << _dataset_description[indexLut].cfid << endl;
				}
				else
				{
					// Mapping Freq
					for(size_t indexFreq = 0; indexFreq < CfShotName.size(); indexFreq++)
					{
						bool isFound = false;
						if(CandidateString.str() == CfShotName[indexFreq])
						{
							//cout << "Freq #" << indexLut << ":" << indexFreq << " = " << CandidateString.str() << " : " << CfShotName[indexFreq] << " : " << CfFreq[indexFreq] << endl;
							_dataset_description[indexLut].freq = CfFreq[indexFreq];
							ComInfoFile << CfFreq[indexFreq] << " ";

							// Mapping Id
							for(size_t indexId = 0; indexId < CfIdName.size(); indexId++)
							{
								if(CfClipName[indexFreq] == CfIdName[indexId])
								{
									isFound = true;
									//cout << "Id #" << indexFreq << ":" << indexId << " " << CfClipName[indexFreq] << " : " << CfIdName[indexId] << " : " << CfId[indexId] << endl;
									_dataset_description[indexLut].cfid = CfId[indexId];
									ComInfoFile << CfId[indexId] << endl;

									break;
								}
							}

							if(!isFound)
							{
								//cout << "Clip not found in cf.id, debug info. " << "Freq #" << indexLut << ":" << indexFreq << " = " << CandidateString.str() << " : " << CfShotName[indexFreq] << " : " << CfFreq[indexFreq] << endl;
								_dataset_description[indexLut].cfid = -1;
								ComInfoFile << "-1" << endl;
							}

							break;
						}
					}
				}

				PrevCandidateString = CandidateString.str();

				if(indexLut%500 == 0)
				{
					cout << 100.0 * indexLut / _dataset_description.size() << "%" << endl;
				}
			}
			ComInfoFile.close();
		}

		// Release
		CfClipName.clear();
		CfShotName.clear();
		CfFreq.clear();
		CfId.clear();
	}
	else
	{
		cout << "file exist...";
		cout.flush();
		ifstream ComInfoFile (ComInfoPath.str().c_str());
		vector<int> CfFreq;
		vector<int> CfId;
		if (ComInfoFile)
		{
			cout << "reading cf.maped...";
			cout.flush();
			while (ComInfoFile.good())
			{
				string line;
				getline(ComInfoFile, line);
				if(line != "")
				{
					vector<string> SpaceSub;
					StringExplode(line, " ", SpaceSub);

					CfFreq.push_back(atoi(SpaceSub[0].c_str()));
					CfId.push_back(atoi(SpaceSub[1].c_str()));
				}
			}
			ComInfoFile.close();

			if(CfFreq.size() != _dataset_description.size())
			{
				cout << "Wrong database version";
				exit(1);
			}
			else
			{
				for(size_t index = 0; index < _dataset_description.size(); index++)
				{
					_dataset_description[index].freq = CfFreq[index];
					_dataset_description[index].cfid = CfId[index];
				}
			}
		}
	}
	cout << "OK!" << endl;
}

void dataset_info::GenFrameFileName(vector<string>& ReturnPaths, const ShotInfo& shot)
{
	for(int shotno = shot.sStart; shotno <= shot.sEnd; shotno+=6)
	{
		stringstream genfile;
		// 2009_08_17_00_002532_0000tbs.jpg
		genfile << shot.year << "_" << ZeroPadNumber(shot.month, 2) << "_" << ZeroPadNumber(shot.day, 2) << "_" << ZeroPadNumber(shot.hour, 2) << "_" << ZeroPadNumber(shotno, 6) << "_" << ZeroPadString(shot.channel, 7) << ".jpg";
		ReturnPaths.push_back(genfile.str());
	}
}

string dataset_info::GenNCheckMiddleFrameFileName(const ShotInfo& shot)
{
	float sampling = 6;

	// cout << "top: " << top << " Result.size(): " << Result.size() << " _dataset_description.size(): " << _dataset_description.size() << " first: " << Result[top].first << endl;
	// ShotInfo shot = _dataset_description[Result[top].first];
	/*int shotDiff = shot.sEnd - shot.sStart + 1; // +1 for filling frame name gap, some ending is different sampling gap
	int midFrame = 0;
	if (shotDiff % 2 == 0)
		midFrame = ((shotDiff / sampling + 1) >> 1) * sampling;
	else
		midFrame = ((shotDiff / sampling + 1) >> 1) * sampling - 1;
	*/ // (((a/b)+1)/2)*b = (a+b)/2
	// (((((b-a)/c)+1)/2)*c) + a = (a+b+c)/2
	// simplified
	int OddEven = ((shot.sEnd - shot.sStart) >> 1) + 1;
	float shotDiffDivSamDiv2 = (shot.sEnd - shot.sStart) / sampling / 2.0;
	int midFrame = 0;
	if(OddEven % 2) // Odd
	midFrame = (int)ceil(floor(shotDiffDivSamDiv2) * sampling + shot.sStart);
	else // Even
	midFrame = (int)floor(ceil(shotDiffDivSamDiv2) * sampling + shot.sStart);

	// TestExisting
	stringstream framefile;
	stringstream genfile;
	struct stat statTmpDir;
	int fail = -1;
	bool secondFail = true;
	do
	{
		fail += 1;
		genfile.str("");
		framefile.str("");
		genfile << shot.year << "_" << ZeroPadNumber(shot.month, 2) << "_" << ZeroPadNumber(shot.day, 2) << "_" << ZeroPadNumber(shot.hour, 2) << "_" << ZeroPadNumber(midFrame - fail, 6) << "_" << ZeroPadString(shot.channel, 7) << ".jpg";
		framefile << ins_root << "/frames/" << shot.path << "/" << genfile.str();
		if(secondFail && fail == 1)
		{
			fail = -7;
			secondFail = false;
		}
		if(fail > 2000)
		{
			cout << "Path incorrupt! (fail:" << fail << ")" << endl;
			genfile.str("");
			genfile << "pathfail.jpg";
			break;
		}
	}while(stat(framefile.str().c_str(), &statTmpDir) != 0);
	/*if(fail != 0)
	{
		cout << "Regen: " << fail  << endl;
		cout << "path:" << framefile.str() << endl;
	}*/
	//if(secondFail)
	//	cout << "Exist! " << framefile.str() << endl;

	return genfile.str();
}

dataset_info::ShotInfo dataset_info::ShotInfoParse(const string& path)
{
	/* Original commercial information
    1:2009_08_17_00_062013_tvtokyo~2009_08_17_00_062500_tvtokyo/2009_08_17_00_062013_tvtokyo~2009_08_17_00_062187_tvtokyo
    */
	/* Position
	0:4 year
	5:2 month
	8:2 day
	11:2 hour
	21:7 channel name
	14:6 clip start
	43:6 clip end
	72:6 shot start
	101:6 shot end
	*/
	ShotInfo ShotInfoResult;

	ShotInfoResult.path = trim_string_until(path, ':'); // 1:xxxx ==> xxxx
	ShotInfoResult.year = atoi(ShotInfoResult.path.substr(0, 4).c_str());
	ShotInfoResult.month = atoi(ShotInfoResult.path.substr(5, 2).c_str());
	ShotInfoResult.day = atoi(ShotInfoResult.path.substr(8, 2).c_str());
	ShotInfoResult.hour = atoi(ShotInfoResult.path.substr(11, 2).c_str());
	// channel
	int i1 = ShotInfoResult.path.rfind('_');
	int i2 = ShotInfoResult.path.length();
	string temp = ShotInfoResult.path.substr(i1+1,i2-i1+1);
	int i3 = temp.rfind('0');
	int i4 = temp.length();
	if (i3==(int)string::npos) i3 = -1;
	ShotInfoResult.channel = temp.substr(i3+1, i4-i3-1);
	ShotInfoResult.cStart = atoi(ShotInfoResult.path.substr(14, 6).c_str());
	ShotInfoResult.cEnd = atoi(ShotInfoResult.path.substr(43, 6).c_str());
	ShotInfoResult.sStart = atoi(ShotInfoResult.path.substr(72, 6).c_str());
	ShotInfoResult.sEnd = atoi(ShotInfoResult.path.substr(101, 6).c_str());

	return ShotInfoResult;
}

void dataset_info::dataset_lookup_table_destroy()
{
	_dataset_description.clear();
	_dataset_frames_dirname.clear();
	_dataset_frames_filename.clear();
}

}
