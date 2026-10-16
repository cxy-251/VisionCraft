#include "LabAlgorithms.h"
#include "ImageOperations.h"
#include <QElapsedTimer>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <algorithm>
#include <cmath>

namespace LabAlgorithms {
namespace {
cv::Mat gray(const cv::Mat &image) {
    cv::Mat out;
    if(image.channels()==1) out=image;
    else cv::cvtColor(image,out,image.channels()==4 ? cv::COLOR_BGRA2GRAY : cv::COLOR_BGR2GRAY);
    return out;
}
cv::Mat color(const cv::Mat &image) {
    cv::Mat out;
    if(image.channels()==1) cv::cvtColor(image,out,cv::COLOR_GRAY2BGR);
    else if(image.channels()==4) cv::cvtColor(image,out,cv::COLOR_BGRA2BGR);
    else out=image.clone();
    return out;
}
int kernel(const QVariantMap &o, const char *key, int fallback) {
    int n=std::clamp(o.value(key,fallback).toInt(),1,51); return n%2 ? n : n+1;
}
int morph(const QString &name) {
    if(name=="Dilate") return cv::MORPH_DILATE;
    if(name=="Erode") return cv::MORPH_ERODE;
    if(name=="Open") return cv::MORPH_OPEN;
    return cv::MORPH_CLOSE;
}
}
// [region match]
Result match(const cv::Mat &source, const cv::Mat &pattern, double threshold)
{
    if(source.empty() || pattern.empty() || pattern.cols>source.cols || pattern.rows>source.rows)
        return {{},{},QStringLiteral("模板不能为空，也不能大于待搜索图像")};
    const auto input=color(source), target=color(pattern);
    cv::Scalar mean, deviation; cv::meanStdDev(target,mean,deviation);
    if(cv::norm(deviation)<1e-6) return {{},{},QStringLiteral("模板为均匀颜色，无法使用归一化相关匹配")};
    cv::Mat scores;
    cv::matchTemplate(input,target,scores,cv::TM_CCOEFF_NORMED);
    double score=0; cv::Point at; cv::minMaxLoc(scores,nullptr,&score,nullptr,&at);
    if(!std::isfinite(score)) return {{},{},QStringLiteral("相关分数无效")};
    auto image=input.clone(); const bool found=score>=threshold;
    if(found) {
        cv::rectangle(image,cv::Rect(at,target.size()),cv::Scalar(0,255,0),3,cv::LINE_AA);
        cv::putText(image,"Match: "+std::to_string(score),{at.x,std::max(25,at.y-8)},cv::FONT_HERSHEY_SIMPLEX,.7,{0,255,0},2,cv::LINE_AA);
    }
    return {image,{{"found",found},{"score",score},{"x",at.x},{"y",at.y},{"width",target.cols},{"height",target.rows}}, {}};
}
// [endregion]
// [region node]
Result node(const cv::Mat &source, const QString &type, const QVariantMap &o)
{
    if(type=="sample") return {graphSample(o.value("hasDefect",true).toBool()),{}, {}};
    if(source.empty()) return {{},{},QStringLiteral("节点缺少输入图像")};
    cv::Mat image; QVariantMap values;
    if(type=="screen") image=source.clone();
    else if(type=="gray") { const auto result=ImageOperations::run(source,ImageOperations::Operation::Grayscale);image=result.image;if(!result.error.isEmpty())return {{},{},result.error}; }
    else if(type=="blur") cv::GaussianBlur(source,image,cv::Size(kernel(o,"ksize",7),kernel(o,"ksize",7)),std::clamp(o.value("sigma",2).toDouble(),0.0,20.0));
    else if(type=="canny") cv::Canny(gray(source),image,std::clamp(o.value("low",50).toDouble(),0.0,255.0),std::clamp(o.value("high",140).toDouble(),0.0,255.0));
    else if(type=="threshold") cv::threshold(gray(source),image,std::clamp(o.value("threshold",128).toDouble(),0.0,255.0),255,cv::THRESH_BINARY|(o.value("otsu",true).toBool() ? cv::THRESH_OTSU : 0));
    else if(type=="morph") cv::morphologyEx(source,image,morph(o.value("op","Close").toString()),cv::getStructuringElement(cv::MORPH_RECT,cv::Size(kernel(o,"ksize",3),kernel(o,"ksize",3))));
    else if(type=="contours") {
        image=color(source); std::vector<std::vector<cv::Point>> contours;
        cv::findContours(gray(source).clone(),contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
        int count=0; double biggest=0;
        const double minimum=std::max(0.0,o.value("minArea",80).toDouble());
        const double maximum=std::max(minimum,o.value("maxArea",80000).toDouble());
        for(const auto &c:contours) {
            const double area=cv::contourArea(c);
            if(area<minimum || area>maximum) continue;
            ++count; biggest=std::max(biggest,area); const auto box=cv::boundingRect(c);
            cv::rectangle(image,box,{16,185,129},2);
            cv::putText(image,"A:"+std::to_string(int(area)),{box.x,std::max(15,box.y-4)},cv::FONT_HERSHEY_SIMPLEX,.45,{56,189,248},1);
        }
        values={{"count",count},{"maxArea",biggest}};
    } else if(type=="qr") {
        image=color(source); cv::QRCodeDetector detector; std::vector<cv::Point2f> points;
        const auto text=detector.detectAndDecode(image,points);
        for(size_t i=0;i<points.size();++i) cv::line(image,points[i],points[(i+1)%points.size()],{0,255,0},3);
        values={{"text",QString::fromStdString(text)},{"located",!points.empty()}};
    } else return {{},{},QStringLiteral("未知节点类型：%1").arg(type)};
    return {image,values,{}};
}
// [endregion]
// [region stream]
Result stream(const cv::Mat &source, const QVariantMap &o)
{
    if(source.empty()) return {{},{},QStringLiteral("没有采集到图像")};
    cv::Mat work=source.clone();
    if(o.value("blur",true).toBool()) cv::GaussianBlur(work,work,cv::Size(kernel(o,"ksize",5),kernel(o,"ksize",5)),0);
    if(o.value("bilateral",false).toBool()) { cv::Mat temp; cv::bilateralFilter(work,temp,7,50,50); work=temp; }
    bool binary=false;
    if(o.value("canny",true).toBool()) { cv::Mat temp; cv::Canny(gray(work),temp,std::clamp(o.value("low",50).toDouble(),0.0,255.0),std::clamp(o.value("high",150).toDouble(),0.0,255.0)); work=temp; binary=true; }
    else if(o.value("adaptive",false).toBool()) { cv::Mat temp; cv::adaptiveThreshold(gray(work),temp,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,11,2); work=temp; binary=true; }
    if(o.value("morph",false).toBool()) cv::morphologyEx(work,work,morph(o.value("op","Close").toString()),cv::getStructuringElement(cv::MORPH_RECT,cv::Size(kernel(o,"morphSize",3),kernel(o,"morphSize",3))));
    cv::Mat image=color(work); int count=0; double biggest=0;
    if(o.value("contours",true).toBool()) {
        cv::Mat bin=gray(work).clone();
        if(!binary) cv::threshold(bin,bin,0,255,cv::THRESH_BINARY|cv::THRESH_OTSU);
        std::vector<std::vector<cv::Point>> contours; cv::findContours(bin,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
        for(const auto &c:contours) {
            const double area=cv::contourArea(c);
            if(area<std::max(0.0,o.value("minArea",80).toDouble())) continue;
            ++count; biggest=std::max(biggest,area); const auto box=cv::boundingRect(c);
            cv::rectangle(image,box,{0,0,255},2,cv::LINE_AA);
            cv::putText(image,std::to_string(count)+":"+std::to_string(int(area)),{box.x,std::max(16,box.y-4)},cv::FONT_HERSHEY_SIMPLEX,.45,{0,255,255},1);
        }
    }
    if(o.value("orb",false).toBool()) {
        std::vector<cv::KeyPoint> points; cv::Mat descriptors;
        cv::ORB::create(150)->detectAndCompute(gray(work),cv::noArray(),points,descriptors);
        cv::drawKeypoints(image,points,image,{0,255,0},cv::DrawMatchesFlags::DRAW_OVER_OUTIMG);
    }
    if(o.value("harris",false).toBool()) {
        cv::Mat response,normal; cv::cornerHarris(gray(work),response,2,3,.04); cv::normalize(response,normal,0,255,cv::NORM_MINMAX,CV_32FC1);
        for(int y=0;y<normal.rows;y+=4) for(int x=0;x<normal.cols;x+=4) if(normal.at<float>(y,x)>145) cv::circle(image,{x,y},4,{0,140,255},-1,cv::LINE_AA);
    }
    return {image,{{"count",count},{"maxArea",biggest},{"passed",count==0},{"inspectionEnabled",o.value("contours",true)}},{}};
}
// [endregion]
cv::Mat mountingSample(int i) {
    // 生成底图
    cv::Mat mat(360, 540, CV_8UC3, cv::Scalar(205, 210, 215));
    cv::rectangle(mat, cv::Rect(40, 30, 460, 300), cv::Scalar(155, 160, 165), -1);
    cv::rectangle(mat, cv::Rect(40, 30, 460, 300), cv::Scalar(70, 75, 80), 3);

    // 四周标准安装孔
    cv::circle(mat, cv::Point(90, 80), 14, cv::Scalar(45, 45, 45), -1);
    cv::circle(mat, cv::Point(450, 80), 14, cv::Scalar(45, 45, 45), -1);
    cv::circle(mat, cv::Point(90, 280), 14, cv::Scalar(45, 45, 45), -1);
    cv::circle(mat, cv::Point(450, 280), 14, cv::Scalar(45, 45, 45), -1);

    // 主装配孔（标准直径约为 110px，即半径 55px）
    int holeRadius = 55;
    // 人为在特定工件中植入真实物理缺陷
    if (i == 4 || i == 11) {
        holeRadius = 66; // 孔径超大严重 NG
    } else if (i == 8 || i == 19) {
        holeRadius = 45; // 孔径偏小严重 NG
    }

    cv::circle(mat, cv::Point(270, 180), holeRadius, cv::Scalar(35, 40, 45), -1);

    // 植入划痕或脏污瑕疵
    if (i == 6 || i == 15 || i == 22) {
        cv::line(mat, cv::Point(290, 100), cv::Point(420, 220), cv::Scalar(25, 25, 210), 3); // 红色严重划痕
    } else if (i == 17) {
        cv::circle(mat, cv::Point(160, 180), 12, cv::Scalar(20, 20, 190), -1); // 异物杂质
    }


    return mat;
}

MountingResult inspectMounting(const cv::Mat &source, double tolerance) {
    QElapsedTimer clock; clock.start(); MountingResult result; result.image=source.clone();
    cv::Mat gray;
    cv::cvtColor(source, gray, cv::COLOR_BGR2GRAY);

    // 二值化求得内部暗孔
    cv::Mat bin;
    cv::threshold(gray, bin, 70, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxHoleRadius = 0.0;
    cv::Point2f holeCenter(0, 0);
    double detectedDefectArea = 0.0;

    for (const auto &c : contours) {
        double a = cv::contourArea(c);
        if (a > 1500) { // 中心孔
            cv::Point2f center;
            float r;
            cv::minEnclosingCircle(c, center, r);
            if (r > maxHoleRadius) {
                maxHoleRadius = r;
                holeCenter = center;
            }
        } else if (a > 40 && a < 1200) {
            // 此工具针对既定装配板：四个标准安装孔不计入表面杂点。
            // 旧版直接累加所有小暗区，导致示例的安装孔也被判成缺陷。
            const cv::Moments moments=cv::moments(c);
            const cv::Point2d center(moments.m10/std::max(1.0,moments.m00),moments.m01/std::max(1.0,moments.m00));
            const double perimeter=cv::arcLength(c,true);
            const double circularity=4*CV_PI*a/std::max(1.0,perimeter*perimeter);
            bool mountingHole=false;
            for(const auto &expected:std::vector<cv::Point2d>{{1.0/6,2.0/9},{5.0/6,2.0/9},{1.0/6,7.0/9},{5.0/6,7.0/9}})
                if(circularity>0.8 && std::abs(center.x/source.cols-expected.x)<0.04 && std::abs(center.y/source.rows-expected.y)<0.04)mountingHole=true;
            if(mountingHole)continue;
            detectedDefectArea += a;
            cv::Rect defectRect = cv::boundingRect(c);
            cv::rectangle(result.image, defectRect, cv::Scalar(0, 0, 255), 2);
            cv::putText(result.image, "DEFECT", cv::Point(defectRect.x, defectRect.y - 4),
                        cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
        }
    }

    // 示例的红色划痕灰度高于70，单独按BGR红色范围提取，不能只依赖暗区阈值。
    cv::Mat redMask;cv::inRange(source,cv::Scalar(0,0,100),cv::Scalar(100,100,255),redMask);
    std::vector<std::vector<cv::Point>> redContours;cv::findContours(redMask,redContours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    for(const auto &contour:redContours){const double area=cv::contourArea(contour);if(area<=40)continue;
        detectedDefectArea+=area;cv::rectangle(result.image,cv::boundingRect(contour),cv::Scalar(0,0,255),2);
    }
    double measuredDia = maxHoleRadius * 2.0;
    result.diameter = measuredDia;
    result.area = detectedDefectArea;

    // 判定逻辑
    bool diaOk = std::abs(measuredDia - 110.0) <= tolerance;
    bool defectOk = (detectedDefectArea <= 80.0);

    if (diaOk && defectOk) {
        result.passed = true;
        result.reason = "检验合格 (ALL OK)";
        // 标注绿色中心孔合格圈
        cv::circle(result.image, cv::Point(static_cast<int>(holeCenter.x), static_cast<int>(holeCenter.y)),
                   static_cast<int>(maxHoleRadius), cv::Scalar(0, 255, 0), 2);
        cv::putText(result.image, QString("Dia:%1px [PASS]").arg(measuredDia, 0, 'f', 1).toStdString(),
                    cv::Point(static_cast<int>(holeCenter.x - 70), static_cast<int>(holeCenter.y + 5)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
    } else {
        result.passed = false;
        if (!diaOk && !defectOk) {
            result.reason = QString("孔径超差 (%1px) & 表面缺陷 (%2px²)").arg(measuredDia, 0, 'f', 1).arg(static_cast<int>(detectedDefectArea));
        } else if (!diaOk) {
            result.reason = QString("孔径尺寸超差 (%1px 偏离标准 %2px)").arg(measuredDia, 0, 'f', 1).arg(110.0, 0, 'f', 1);
        } else {
            result.reason = QString("检出表面划痕/脏污缺陷 (%1 px²)").arg(static_cast<int>(detectedDefectArea));
        }
        // 标注红色中心孔异常圈
        cv::circle(result.image, cv::Point(static_cast<int>(holeCenter.x), static_cast<int>(holeCenter.y)),
                   static_cast<int>(maxHoleRadius), cv::Scalar(0, 0, 255), 3);
        cv::putText(result.image, QString("Dia:%1px [NG]").arg(measuredDia, 0, 'f', 1).toStdString(),
                    cv::Point(static_cast<int>(holeCenter.x - 65), static_cast<int>(holeCenter.y + 5)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
    }


    result.ms=clock.nsecsElapsed()/1e6; return result;
}

cv::Mat graphSample(bool defect) {
    cv::Mat image;
    // 生成工业工件金属板合成图像
    image = cv::Mat(400, 600, CV_8UC3, cv::Scalar(210, 215, 220));

    // 绘制工件外轮廓倒角矩形
    cv::rectangle(image, cv::Rect(50, 40, 500, 320), cv::Scalar(160, 165, 170), -1);
    cv::rectangle(image, cv::Rect(50, 40, 500, 320), cv::Scalar(80, 85, 90), 3);

    // 中央主装配通孔
    cv::circle(image, cv::Point(300, 200), 55, cv::Scalar(40, 45, 50), -1);
    cv::circle(image, cv::Point(300, 200), 65, cv::Scalar(110, 115, 120), 2);

    // 四周螺丝沉孔
    cv::circle(image, cv::Point(100, 90), 16, cv::Scalar(50, 50, 50), -1);
    cv::circle(image, cv::Point(500, 90), 16, cv::Scalar(50, 50, 50), -1);
    cv::circle(image, cv::Point(100, 310), 16, cv::Scalar(50, 50, 50), -1);
    cv::circle(image, cv::Point(500, 310), 16, cv::Scalar(50, 50, 50), -1);

    // 可选划痕缺陷
    if (defect) {
        cv::line(image, cv::Point(320, 130), cv::Point(440, 240), cv::Scalar(30, 30, 220), 3);
        cv::circle(image, cv::Point(190, 210), 9, cv::Scalar(20, 20, 200), -1);
    }

    // 激光雕刻文字
    cv::putText(image, "PART-QC-2026-OK", cv::Point(180, 75), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(50, 50, 50), 2);

    return image;
}
}
