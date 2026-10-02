// cv::Mat 按值传参时，函数里改像素会不会影响调用者？
// 运行：./example_opencv_mat_sharing
#include <opencv2/core.hpp>
#include <cstdio>

// [region functions]
static void darkenInPlace(cv::Mat img) { img -= cv::Scalar::all(50); }      // 原地减
static void darkenExpr(cv::Mat img) { img = img - cv::Scalar::all(50); }    // 先算表达式再赋值
static void darkenInt(cv::Mat img) { img -= 50; }                           // 减一个整数
// [endregion]

static void show(const char *what, const cv::Mat &photo)
{
    const cv::Vec3b v = photo.at<cv::Vec3b>(0, 0);
    std::printf("%-30s 调用后 photo 的 B G R = %3d %3d %3d\n", what, v[0], v[1], v[2]);
}

int main()
{
    // [region main]
    const cv::Mat original(2, 2, CV_8UC3, cv::Scalar(200, 200, 200));
    cv::Mat photo;

    photo = original.clone(); darkenInPlace(photo);         show("darkenInPlace(photo)", photo);
    photo = original.clone(); darkenExpr(photo);            show("darkenExpr(photo)", photo);
    photo = original.clone(); darkenInt(photo);             show("darkenInt(photo)", photo);
    photo = original.clone(); darkenInPlace(photo.clone()); show("darkenInPlace(photo.clone())", photo);
    // [endregion]
    return 0;
}
