#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "faceAnalysis/faceAnalysis.hpp"
#include "kcf/kcftracker.hpp"
#include "utils_config.hpp"
#include "dataBase.hpp"
#include "kdtree.hpp"


#include <lshbox.h>
#include<ctime>

using namespace cv;
using namespace RESIDEO;

int trackingGap = 60;

mapFaceCollectDataSet getmapDatafaceBase(FaceBase &dataColletcion){
	mapFaceCollectDataSet dataTestSet;
	FaceBase::iterator it;
	std::cout<<"************************"<<std::endl;
	for(it = dataColletcion.begin(); it != dataColletcion.end(); it++){
		int gender = it->first;
		vector_feature feature = it->second;
		for(int i = 0; i < feature.size(); i++){

		}
		mapFeature subfeature;
		
		if(dataTestSet.find(gender) == dataTestSet.end()){
			for(int j = 0; j < feature.size(); j++){
				std::cout<<"gender: "<<gender<<" j: "<<j<<std::endl;
				subfeature.insert(std::make_pair(feature[j].second, feature[j].first));
			}
			std::cout<<"feature size: "<<subfeature.size()<<std::endl;
			dataTestSet.insert(std::make_pair(gender, subfeature));
		}
	}
	int num = 0;
	mapFaceCollectDataSet::iterator iter;
	for(iter = dataTestSet.begin(); iter != dataTestSet.end(); iter++){
		mapFeature subfeature = iter->second;
		num += subfeature.size();
	}
	std::cout<<"map num: "<<num<<std::endl;
	return dataTestSet;
}

std::vector<lshbox::dataUnit> getlshDataset(FaceBase dataColletcion){
		FaceBase::iterator iter;
		std::vector<lshbox::dataUnit> dataSet;
		for(iter = dataColletcion.begin(); iter != dataColletcion.end(); iter++){
			vector_feature feature = iter->second;
			for(int j = 0; j < feature.size(); j++){
				dataSet.push_back(std::make_pair(feature[j].second.featureFace, feature[j].first));
			}
		}
		return dataSet;
}

/************************以上测试map*********************************/
int main(int argc, char* argv[]){
	faceAnalysis faceInfernece;
	dataBase baseface(faceDir, facefeaturefile);
#if 0
	baseface.generateBaseFeature(faceInfernece);
#else
	FaceBase dataColletcion = baseface.getStoredDataBaseFeature(facefeaturefile);

	#ifdef KDTREE_SEARCH
	std::map<int, KDtype >trainData;
	FaceBase::iterator iter;
	int gender = 0;
	for(iter = dataColletcion.begin(); iter != dataColletcion.end(); iter++){
		gender = iter->first;
		vector_feature feature = iter->second;
		for(int j = 0; j < feature.size(); j++){
			if(trainData.find(gender) == trainData.end()){
				KDtype new_feature;
				new_feature.push_back(std::make_pair(feature[j].second.featureFace, feature[j].first));
				trainData.insert(std::make_pair(gender, new_feature));
			}else{
				KDtype feature_list = trainData.find(gender)->second;
				feature_list.push_back(std::make_pair(feature[j].second.featureFace, feature[j].first));
				trainData[gender] = feature_list;
			}			
		}
	}
	KDtreeNode *male_kdtree = new KDtreeNode;
	KDtreeNode *female_kdtree = new KDtreeNode;
	buildKdtree(male_kdtree, trainData.find(0)->second, 0);
	buildKdtree(female_kdtree, trainData.find(1)->second, 0);
	#endif
	#ifdef LSH_SEARCH
	lshbox::featureUnit lshgoal;
	std::vector<lshbox::dataUnit> lshDataSet = getlshDataset(dataColletcion);
	std::string file = "lsh.binary";
	bool use_index = false;
    lshbox::PSD_VECTOR_LSH<float> mylsh;
    if (use_index)
    {
        mylsh.load(file);
    }else{
        lshbox::PSD_VECTOR_LSH<float>::Parameter param;
        param.M = 521;
        param.L = 5;
        param.D = 512;
        param.T = GAUSSIAN;
        param.W = 0.5;
		mylsh.reset(param);
        mylsh.hash(lshDataSet);
        mylsh.save(file);
    }
    lshbox::Metric<float> metric(512, L2_DIST);
	#endif
	KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);
    /**********************初始化跟踪******************/
	Mat frame;
	Rect result;
	int FrameIdx = 0;
	int nFrames = 0;
	VideoCapture cap(0);  
    if(!cap.isOpened())  
    {  
        return -1;  
    }
    bool stop = false;
	RecognResultTrack resutTrack;
    while(!stop)  
    {  
        cap>>frame;
		int width = frame.cols;
		int height = frame.rows;
		int nDataBaseSize = 0;
		if(FrameIdx % trackingGap == 0){
			resutTrack.clear();
			std::vector<faceAnalysisResult> result= faceInfernece.faceInference(frame, detMargin, 20.0f);
			string person = "unknown man";
			
			for(int ii = 0; ii < result.size(); ii++){
				if(result[ii].haveFeature){
					encodeFeature detFeature = result[ii].faceFeature;
					#ifdef KDTREE_SEARCH
					std::pair<float, std::string > nearestNeighbor;
					if(result[ii].faceAttri.gender==0)
						nearestNeighbor = searchNearestNeighbor(detFeature.featureFace, male_kdtree);
					else{
						nearestNeighbor = searchNearestNeighbor(detFeature.featureFace, female_kdtree);	
					}
					person = nearestNeighbor.second;
					if(nearestNeighbor.first > euclideanValueThresold){
						person = "unknown man";
					}
					#endif
					#ifdef LOOP_SEARCH
					std::pair<float, std::string>nearestNeighbor= serachCollectDataNameByloop(dataColletcion,
             															detFeature, result[ii].faceAttri.gender);
					person = nearestNeighbor.second;
					if(nearestNeighbor.first < cosValueThresold){
						person = "unknown man";
					}
					#endif
					#ifdef LSH_SEARCH
					lshgoal = result[ii].faceFeature.featureFace;
					std::pair<float, std::string>nearestNeighbor = mylsh.query(lshgoal, metric, lshDataSet);
					person = nearestNeighbor.second;
					if(nearestNeighbor.first > euclideanValueThresold){
						person = "unknown man";
					}
					#endif
					#ifdef USE_TRACKING
					detBoxInfo trackBoxInfo;
					trackBoxInfo.detBox.xmin = result[ii].faceBox.xmin;
					trackBoxInfo.detBox.xmax = result[ii].faceBox.xmax;
					trackBoxInfo.detBox.ymin = result[ii].faceBox.ymin;
					trackBoxInfo.detBox.ymax = result[ii].faceBox.ymax;
					trackBoxInfo.name = person;
					resutTrack.push_back(trackBoxInfo);
					#endif
				}
				#if DEBUG
				box detBox = result[ii].faceBox;
                cv::rectangle( frame, cv::Point( detBox.xmin, detBox.ymin ), 
											cv::Point( detBox.xmax, detBox.ymax), 
															cv::Scalar( 0, 255, 255 ), 1, 8 );
				cv::putText(frame, person.c_str(), cv::Point( detBox.xmin, detBox.ymin ), 
					cv::FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 255), 2, 8, 0);
				cv::Mat roiImage = frame(cv::Rect(detBox.xmin, detBox.ymin, detBox.xmax - detBox.xmin, detBox.ymax - detBox.ymin));
				for(unsigned i = 0; i < 5; i++){
					cv::circle(roiImage, cv::Point(result[ii].faceAttri.landmarks[i].point_x, result[ii].faceAttri.landmarks[i].point_y)
								, 3, cv::Scalar(0, 0, 213), -1);
				}
				std::string title = labelGender[result[ii].faceAttri.gender] + std::string(", ") + labelGlass[result[ii].faceAttri.glass];
				cv::putText(frame, title, cv::Point( detBox.xmin + 40, detBox.ymin + 40 ), 
					cv::FONT_ITALIC, 0.6, Scalar(0, 255, 0), 1);
				#endif
			}
		}else{
			for(int ii = 0; ii <resutTrack.size(); ii++){//tracking
				int xMin = resutTrack[ii].detBox.xmin;
				int yMin = resutTrack[ii].detBox.ymin;
				int width_ = resutTrack[ii].detBox.xmax - resutTrack[ii].detBox.xmin;
				int height_ = resutTrack[ii].detBox.ymax - resutTrack[ii].detBox.ymin;
				if (nFrames == 0) {
					tracker.init( Rect(xMin, yMin, width_, height_), frame );
					rectangle( frame, Point( xMin, yMin ), Point( xMin+width_, yMin+height_), 
													Scalar( 0, 255, 255 ), 1, 8 );
				}else{// update position info
					result = tracker.update(frame);
					#if USE_TRACKING
					rectangle( frame, Point( result.x, result.y ), 
											Point( result.x+result.width, result.y+result.height), 
															Scalar( 0, 255, 255 ), 1, 8 );
					cv::putText(frame, resutTrack[ii].name, cv::Point( result.x, result.y), 
						cv::FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 255), 2, 8, 0);
					#endif
				}
				
			}	
			nFrames++;
		}

		FrameIdx++;
		if(FrameIdx == trackingGap){
			FrameIdx = 0;
			nFrames = 0;
		}
        imshow("faceRecognition",frame);
        if(waitKey(1) > 0)  
            stop = true;
    }
	cap.release();
#endif
}
