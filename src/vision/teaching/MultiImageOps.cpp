#include "MultiImageOps.h"
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace teaching {
namespace {
cv::Mat linear(const cv::Mat &encoded) {
    cv::Mat f; encoded.convertTo(f,CV_32FC3,1.0/255); cv::pow(f,2.2,f); return f;
}
cv::Mat display(const cv::Mat &linearImage) {
    cv::Mat f; cv::max(linearImage,0,f); cv::min(f,1,f); cv::pow(f,1.0/2.2,f);
    cv::Mat out; f.convertTo(out,CV_8UC3,255); return out;
}
cv::Mat channels3(const cv::Mat &m) { cv::Mat out; cv::merge(std::vector<cv::Mat>{m,m,m},out); return out; }
cv::Mat texture(int preset) {
    cv::Mat scene(240,480,CV_8UC3,cv::Scalar(90,100,120));
    if (preset == 5) return scene;
    if (preset == 6) {
        for (int y=0;y<240;y+=24) for (int x=0;x<480;x+=24)
            if ((x/24+y/24)%2) cv::rectangle(scene,{x,y,24,24},{190,190,190},-1);
        return scene;
    }
    cv::RNG rng(20261017);
    for (int i=0;i<140;++i) {
        cv::Point center(rng.uniform(5,475),rng.uniform(5,235));
        cv::circle(scene,center,rng.uniform(2,10),cv::Scalar(rng.uniform(10,240),rng.uniform(10,240),rng.uniform(10,240)),-1);
    }
    cv::rectangle(scene,{210,55,65,65},{0,0,0},-1); // valid black pixels inside overlap
    cv::putText(scene,"KNOWN PLANE",{95,200},cv::FONT_HERSHEY_SIMPLEX,.7,{225,235,240},2);
    if(preset==2) cv::rectangle(scene,{120,30,240,160},{0,0,0},-1);
    return scene;
}
bool cancelled(const std::shared_ptr<std::atomic_bool> &c) { return c && c->load(); }
void stopIfCancelled(const std::shared_ptr<std::atomic_bool> &c) {
    if (cancelled(c)) throw std::runtime_error("Cancelled between algorithm steps");
}
// [region multiband]
cv::Mat multiband(const cv::Mat &a,const cv::Mat &b,const cv::Mat &ma,const cv::Mat &mb,
                 const cv::Mat &wa,const cv::Mat &wb,int levels) {
    auto imagePyramid=[levels](cv::Mat image,cv::Mat mask) {
        std::vector<cv::Mat> pyramid{image};
        for (int i=0;i<levels;++i) {
            cv::Mat numerator,nextMask,next;
            cv::pyrDown(image.mul(channels3(mask)),numerator);
            cv::pyrDown(mask,nextMask);
            cv::Mat safe; cv::max(nextMask,1e-6,safe);
            cv::divide(numerator,channels3(safe),next);
            pyramid.push_back(next); image=next; mask=nextMask;
        }
        return pyramid;
    };
    auto pa=imagePyramid(a,ma), pb=imagePyramid(b,mb);
    std::vector<cv::Mat> weightsA{wa},weightsB{wb};
    for(int i=0;i<levels;++i) {
        cv::Mat x,y; cv::pyrDown(weightsA.back(),x); cv::pyrDown(weightsB.back(),y);
        weightsA.push_back(x); weightsB.push_back(y);
    }
    cv::Mat result;
    for(int i=levels;i>=0;--i) {
        cv::Mat la=pa[i],lb=pb[i];
        if(i<levels) {
            cv::Mat upA,upB; cv::pyrUp(pa[i+1],upA,pa[i].size()); cv::pyrUp(pb[i+1],upB,pb[i].size());
            la=pa[i]-upA; lb=pb[i]-upB;
        }
        cv::Mat sum,safe,x,y;
        sum=weightsA[i]+weightsB[i]; cv::max(sum,1e-6,safe);
        cv::divide(weightsA[i],safe,x); cv::divide(weightsB[i],safe,y);
        cv::Mat band=la.mul(channels3(x))+lb.mul(channels3(y));
        if(result.empty()) result=band;
        else { cv::Mat up; cv::pyrUp(result,up,band.size()); result=up+band; }
    }
    return result;
}
// [endregion]
// [region exposure]
MultiImageResult exposures() {
    MultiImageResult out;
    cv::Mat radiance(240,320,CV_32FC3);
    for(int y=0;y<radiance.rows;++y) for(int x=0;x<radiance.cols;++x) {
        const float value=float(std::pow(2.0,-5.0+8.0*x/(radiance.cols-1)));
        const float stripe=(y/12)%2 ? 1.f : .55f;
        radiance.at<cv::Vec3f>(y,x)={value*stripe*.7f,value*stripe,value*stripe*.85f};
    }
    const std::vector<float> times{.125f,.5f,2.f};
    std::vector<cv::Mat> images;
    for(float t:times) images.push_back(display(radiance*t));
    cv::Mat fusion; cv::createMergeMertens()->process(images,fusion);
    // Known response g(z)=linear intensity; the generator uses gamma 2.2 and clips to 8 bits.
    cv::Mat response(256,1,CV_32FC3);
    for(int z=0;z<256;++z) response.at<cv::Vec3f>(z)=cv::Vec3f::all(float(std::pow(std::max(z,1)/255.0,2.2)));
    cv::Mat hdr; cv::createMergeDebevec()->process(images,hdr,cv::Mat(times),response);
    cv::Mat mapped; cv::createTonemapReinhard(2.2f)->process(hdr,mapped);
    cv::patchNaNs(mapped,0); cv::max(mapped,0,mapped); cv::min(mapped,1,mapped);
    cv::Mat fusion8,mapped8;
    cv::max(fusion,0,fusion); cv::min(fusion,1,fusion);
    fusion.convertTo(fusion8,CV_8UC3,255); mapped.convertTo(mapped8,CV_8UC3,255);
    out.images={{"exposure-0.125s",images[0]},{"exposure-0.5s",images[1]},
                {"exposure-2s",images[2]},{"mertens-display",fusion8},{"hdr-tonemapped",mapped8}};
    double maximum=0; cv::minMaxLoc(hdr.reshape(1),nullptr,&maximum);
    std::ostringstream text;
    text<<"Generated linear radiance; gamma=2.2; times=0.125,0.5,2 seconds. "
        <<"Debevec uses KNOWN response (not estimated calibration). HDR max="<<maximum
        <<"; Mertens display and Reinhard tone mapping are different outputs.";
    out.summary=text.str(); return out;
}
// [endregion]
}
MultiImageResult composeLesson(const MultiImageInput &p,const std::shared_ptr<std::atomic_bool> &cancel) {
    MultiImageResult out;
    try {
        if(p.preset<0 || p.preset>7 || p.method<0 || p.method>2 || p.levels<1 || p.levels>5)
            throw std::invalid_argument("Preset, method or pyramid level out of range");
        stopIfCancelled(cancel);
        if(p.preset==7) return exposures();
        const cv::Mat scene=texture(p.preset);
        const cv::Mat a=scene(cv::Rect(0,0,320,240)).clone();
        cv::Mat b=scene(cv::Rect(160,0,320,240)).clone();
        if(p.preset==1) b=display(linear(b)*1.7);
        if(p.preset==3) { // shifted foreground in only the second view: two-depth parallax surrogate
            cv::rectangle(b,{100,65,55,70},{40,40,220},-1);
        }
        if(p.preset==4) cv::circle(b,{95,160},24,{30,220,220},-1);
        cv::Mat h=(cv::Mat_<double>(3,3)<<1,0,160,0,1,0,0,0,1);
        // [region alignment]
        if(p.estimateAlignment) {
            auto orb=cv::ORB::create(1000); std::vector<cv::KeyPoint> ka,kb; cv::Mat da,db;
            orb->detectAndCompute(a,cv::noArray(),ka,da); orb->detectAndCompute(b,cv::noArray(),kb,db);
            if(da.empty()||db.empty()) throw std::runtime_error("No descriptors: low texture cannot determine alignment");
            std::vector<std::vector<cv::DMatch>> pairs; cv::BFMatcher(cv::NORM_HAMMING).knnMatch(db,da,pairs,2);
            std::vector<cv::Point2f> source,target;
            for(const auto &pair:pairs) if(pair.size()==2 && pair[0].distance < .75f*pair[1].distance) {
                source.push_back(kb[pair[0].queryIdx].pt); target.push_back(ka[pair[0].trainIdx].pt);
            }
            if(source.size()<4) throw std::runtime_error("Fewer than four candidate correspondences");
            cv::Mat inliers; h=cv::findHomography(source,target,cv::RANSAC,3,inliers);
            if(h.empty() || !cv::checkRange(h)) throw std::runtime_error("Homography is not finite or cannot be estimated");
            out.inliers=cv::countNonZero(inliers);
            if(out.inliers<4) throw std::runtime_error("Fewer than four geometric inliers");
        }
        // [endregion]
        stopIfCancelled(cancel);
        // [region canvas]
        // Pixel-edge rectangle, not merely last pixel centers; output origin is rounded down.
        const std::vector<cv::Point2d> edges{{0,0},{320,0},{320,240},{0,240}};
        double minX=0,minY=0,maxX=320,maxY=240;
        double sign=0;
        for(const auto &v:edges) {
            const cv::Vec3d q=cv::Matx33d(h)*cv::Vec3d(v.x,v.y,1);
            if(std::abs(q[2])<1e-8 || (sign!=0 && sign*q[2]<0))
                throw std::runtime_error("Projected rectangle crosses the projective horizon");
            sign=q[2]; const double x=q[0]/q[2],y=q[1]/q[2];
            minX=std::min(minX,x); minY=std::min(minY,y); maxX=std::max(maxX,x); maxY=std::max(maxY,y);
        }
        minX=std::floor(minX); minY=std::floor(minY); maxX=std::ceil(maxX); maxY=std::ceil(maxY);
        if(!std::isfinite(minX+minY+maxX+maxY) || maxX-minX>2048 || maxY-minY>2048
           || std::abs(minX)>100000 || std::abs(minY)>100000)
            throw std::runtime_error("Canvas exceeds bounded teaching size");
        const cv::Size size(int(maxX-minX),int(maxY-minY));
        out.canvasOrigin={int(minX),int(minY)};
        const cv::Mat t=(cv::Mat_<double>(3,3)<<1,0,-minX,0,1,-minY,0,0,1);
        cv::Mat wa,wb,ma,mb;
        cv::warpPerspective(linear(a),wa,t,size,cv::INTER_LINEAR);
        cv::warpPerspective(linear(b),wb,t*h,size,cv::INTER_LINEAR);
        const cv::Mat valid(a.size(),CV_8U,cv::Scalar(255));
        cv::warpPerspective(valid,ma,t,size,cv::INTER_NEAREST);
        cv::warpPerspective(valid,mb,t*h,size,cv::INTER_NEAREST);
        cv::Mat overlap=ma & mb, united=ma | mb;
        out.overlapPixels=cv::countNonZero(overlap);
        if(!out.overlapPixels) throw std::runtime_error("No valid overlap");
        cv::Mat diff; cv::absdiff(wa,wb,diff); const cv::Scalar e=cv::mean(diff,overlap);
        out.overlapMae=(e[0]+e[1]+e[2])/3;
        // [endregion]
        stopIfCancelled(cancel);
        // [region feather]
        auto distanceWeight=[](const cv::Mat &mask) {
            cv::Mat padded,dist; cv::copyMakeBorder(mask,padded,1,1,1,1,cv::BORDER_CONSTANT,0);
            cv::distanceTransform(padded,dist,cv::DIST_L2,3);
            return dist(cv::Rect(1,1,mask.cols,mask.rows)).clone();
        };
        cv::Mat weightA=distanceWeight(ma),weightB=distanceWeight(mb),sum,safe;
        sum=weightA+weightB; cv::max(sum,1e-6,safe);
        cv::divide(weightA,safe,weightA); cv::divide(weightB,safe,weightB);
        cv::Mat result;
        if(p.method==0) { result=wa.clone(); wb.copyTo(result,mb); }
        else if(p.method==1) result=wa.mul(channels3(weightA))+wb.mul(channels3(weightB));
        else {
            cv::Mat fa,fb; ma.convertTo(fa,CV_32F,1.0/255); mb.convertTo(fb,CV_32F,1.0/255);
            result=multiband(wa,wb,fa,fb,weightA,weightB,p.levels);
        }
        result.setTo(cv::Scalar(),united==0);
        // [endregion]
        cv::Mat weights; weightB.convertTo(weights,CV_8U,255);
        cv::Mat errorView; diff.convertTo(errorView,CV_8UC3,255);
        out.images={{"input-A",a},{"input-B",b},{"aligned-A",display(wa)},{"aligned-B",display(wb)},
                    {"valid-overlap",overlap},{"weight-B",weights},{"linear-absolute-difference",errorView},{"result",display(result)}};
        std::ostringstream text;
        text<<"canvas="<<size.width<<"x"<<size.height<<" origin="<<out.canvasOrigin.x<<","<<out.canvasOrigin.y
            <<" overlap="<<out.overlapPixels<<" linear MAE="<<out.overlapMae
            <<" inliers="<<out.inliers<<" alignment="<<(p.estimateAlignment ? "estimated (not truth)" : "known plane");
        if(p.preset==3) text<<"; displaced foreground patch is a controlled mismatch, not a physical 3D renderer";
        if(p.preset==4) text<<"; moving object cannot be repaired by blending";
        if(p.preset==5 || p.preset==6) text<<"; known H remains available even when appearance does not determine H";
        out.summary=text.str();
    } catch(const cv::Exception &e) { out.error=e.what(); }
      catch(const std::exception &e) { out.error=e.what(); }
    return out;
}
}
