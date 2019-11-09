#ifndef _RESIDEO_UTILS_HPP_
#define _RESIDEO_UTILS_HPP_
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <stdlib.h>
#include <assert.h>
const float cosValueThresold = 0.65f;
const float euclideanValueThresold = 0.25f;

typedef struct modelParameter_{
    std::string m_model_weight_;
    std::string m_model_prototxt_;
    float m_std_value_;
    float m_mean_value_[3];
}modelParameter;

struct encodeFeature{
        std::vector<float> featureFace;
};
struct featureCmp{
    bool operator()(const encodeFeature &leftValue, const encodeFeature &rightValue) const{
        float top =0.0f, bottomLeft=0.0f, bottomRight=0.0f, EuclideanValue = 0.0f;

        assert(leftValue.featureFace.size()==rightValue.featureFace.size());
        assert(leftValue.featureFace.size() == 512);

        for(int ii = 0; ii < 512; ii++){
            top += leftValue.featureFace[ii]*rightValue.featureFace[ii];
            bottomLeft += leftValue.featureFace[ii]*leftValue.featureFace[ii];
            bottomRight += rightValue.featureFace[ii]*rightValue.featureFace[ii];
            EuclideanValue += std::pow((leftValue.featureFace[ii]-rightValue.featureFace[ii]),2);
        }
        
        float cosValue = (float) (top/(sqrt(bottomLeft)*sqrt(bottomRight)));
        float Euclidean = std::sqrt(EuclideanValue);
        printf("EuclideanValue: %f, cosValue: %f\n", Euclidean, cosValue);
        if(cosValue > cosValueThresold && Euclidean < euclideanValueThresold){
            return false;
        }else{
            if(bottomLeft != bottomRight){
                return bottomLeft < bottomRight;
            }else{
                return (bottomLeft + 0.00025) > bottomRight;
            }
        }
    }
};
//typedef std::map<encodeFeature, std::string, featureCmp> mapFeature;
typedef std::vector<std::pair<std::string, encodeFeature > >vector_feature;
typedef std::map<int,  vector_feature> FaceBase;

typedef struct resideo_point_{
    int point_x;
    int point_y;
}point;

typedef struct Angle_{
    float yaw;
    float pitch;
    float roll;
}angle;

typedef struct faceAttri_{
    int gender;
    std::vector<point> landmarks;
    angle facepose;
}faceattribute;


typedef struct Box_{
    int xmin;
    int ymin;
    int xmax;
    int ymax;
}box;
typedef std::pair<float, box> output;
typedef std::vector<float> Prediction;

typedef struct _faceAnalysis_result{
    box faceBox;
    faceattribute faceAttri;
    encodeFeature faceFeature;
    bool haveFeature;
} faceAnalysisResult;

typedef struct detBoxInfo_{
	box detBox;
	std::string name;
}detBoxInfo;

typedef std::vector<detBoxInfo> RecognResultTrack;


/******************静态函数******************************/
static float compareDistance(const encodeFeature &leftValue, const encodeFeature &rightValue){
    float top =0.0f, bottomLeft=0.0f, bottomRight=0.0f, euclideanValue = 0.0f;

    assert(leftValue.featureFace.size()==rightValue.featureFace.size());
    assert(leftValue.featureFace.size() == 512);

    for(int ii = 0; ii < 512; ii++){
        top += leftValue.featureFace[ii]*rightValue.featureFace[ii];
        bottomLeft += leftValue.featureFace[ii]*leftValue.featureFace[ii];
        bottomRight += rightValue.featureFace[ii]*rightValue.featureFace[ii];
        euclideanValue += std::pow((leftValue.featureFace[ii]-rightValue.featureFace[ii]),2);
    }
    
    float cosValue = (float) (top/(sqrt(bottomLeft)*sqrt(bottomRight)));
    float Euclidean = std::sqrt(euclideanValue);
    printf("euclideanValue: %f, cosValue: %f\n", Euclidean, cosValue);
    return cosValue;
}

static float computeDistance(const std::vector<float> leftValue, const std::vector<float> &rightValue, 
                                unsigned int method){
    float top =0.0f, bottomLeft=0.0f, bottomRight=0.0f, euclideanValue = 0.0f;

    assert(leftValue.size()==rightValue.size());
    assert(leftValue.size() == 512);

    for(int ii = 0; ii < 512; ii++){
        top += leftValue[ii]*rightValue[ii];
        bottomLeft += leftValue[ii]*leftValue[ii];
        bottomRight += rightValue[ii]*rightValue[ii];
        euclideanValue += std::pow((leftValue[ii]-rightValue[ii]),2);
    }
    
    float cosValue = (float) (top/(sqrt(bottomLeft)*sqrt(bottomRight)));
    float Euclidean = std::sqrt(euclideanValue);
    switch (method)
    {
    case 0: //euclideanDistance
        return Euclidean;
        break;
    case 1: //cosDistance
        return cosValue;
        break;
    default:
        break;
    }
    
}

static std::string getCollectDataName(dataBase dataColletcion, std::vector<float>feature){
    std::string person;
    for(int i = 0; i < dataColletcion.size(); i++){
		vector_feature feature = dataColletcion[i];
		for(int j = 0; j < feature.size(); j++){
			if(computeDistance(feature, feature[j].second.featureFace, 0)==0)
                person = feature[j].first;
		}
	}
    return person;
}

/******************初始化网络模型*************************/

static std::string faceDir = "../faceBase";
static std::string facefeaturefile = "../savefeature.txt";

static modelParameter detParam ={
    .m_model_weight_ = "../model/face_detector.caffemodel",
    .m_model_prototxt_ = "../model/face_detector.prototxt",
    .m_std_value_ = 0.007845,
    {103.94, 116.78, 123.68}
};
static modelParameter attriParam ={
    .m_model_weight_ = "../model/facelandmark.caffemodel",
    .m_model_prototxt_ = "../model/facelandmark.prototxt",
    .m_std_value_ = 0.007845,
    {127.5, 127.5, 127.5}
};
static modelParameter facenetParam ={
    .m_model_weight_ = "../model/facenet.caffemodel",
    .m_model_prototxt_ = "../model/facenet.prototxt",
    .m_std_value_ = 0.007845,
    {127.5, 127.5, 127.5}
};
/*****************static variables******************/
static int detMargin = 32;
static float confidencethreold = 0.35;
static char *labelGender[] = {"male", "female"};
/****************初始化跟踪模块***********************/
static bool HOG = true;
static bool FIXEDWINDOW = false;
static bool MULTISCALE = true;
static bool LAB = false;
#endif