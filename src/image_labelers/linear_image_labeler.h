// Copyright (C) 2017  I. Bogoslavskyi, C. Stachniss, University of Bonn

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SRC_IMAGE_LABELERS_LINEAR_IMAGE_LABELER_H_
#define SRC_IMAGE_LABELERS_LINEAR_IMAGE_LABELER_H_

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <queue>

#include "image_labelers/abstract_image_labeler.h"
#include "image_labelers/pixel_coords.h"

namespace depth_clustering {

/**
 * @brief      Class for linear image labeler.
 *
 * @tparam     STEP_ROW  step we do over image over rows
 * @tparam     STEP_COL  step we do over image over cols
 */
template <int16_t STEP_ROW = 1, int16_t STEP_COL = 1>
class LinearImageLabeler : public AbstractImageLabeler {
 public:
  static constexpr int16_t NEIGH_SIZE = 2 * STEP_ROW + 2 * STEP_COL;
  std::array<PixelCoord, NEIGH_SIZE> Neighborhood;
 
  /**
   * @brief      Initialize Linear image labeler
   *
   * @param[in]  depth_image      The depth image
   * @param[in]  params           The projection parameters
   * @param[in]  angle_threshold  The angle threshold to seaparate clusters
   */
  explicit LinearImageLabeler(const cv::Mat& depth_image,
                              const ProjectionParams& params,
                              const Radians& angle_threshold)
      : AbstractImageLabeler(depth_image, params, angle_threshold) {
    // this can probably be done at compile time
    int16_t counter = 0;
    std::cout<<" in the liner_iamge_labler.h now it it in the liner_image_function the value of STEP_ROW is :"<<STEP_ROW <<std::endl;
    std::cout<<"in the liner_iamge_labler.h now it it in the liner_image_function the value of STEP_COL; is :"<< STEP_COL<<std::endl;
    for (int16_t r = STEP_ROW; r > 0; --r) {
      //std::cout<<"Now it runs in the for STEP_ROW loop it is in the liner_image_lable.h"<<std::endl;
      //from here it turn into the pixelcoord.h
      Neighborhood[counter++] = PixelCoord(-r, 0);
    //  std::cout<<"the value of the is "<<std::endl;
      Neighborhood[counter++] = PixelCoord(r, 0);
    }
    for (int16_t c = STEP_COL; c > 0; --c) {
      Neighborhood[counter++] = PixelCoord(0, -c);
      Neighborhood[counter++] = PixelCoord(0, c);//在这里定义四邻域　分别为上下左右。
    }
  }
  virtual ~LinearImageLabeler() {}

  /**
   * @brief      Label a single connected component with BFS. Can be done faster
   *             if we augment the queue with a hash or smth.
   * //如果我们采用BFS
   * @param[in]  label        Label this component with this label
   * @param[in]  start        Start pixel
   * @param[in]  diff_helper  The difference helper DiffAt
   */
  void LabelOneComponent(uint16_t label, const PixelCoord& start,
                         const AbstractDiff* diff_helper) {
    ///std::cout<<"Now it runs in the LabelOneComponent function  in the liner_image_lable .h"<<std::endl;
    // breadth first search
    std::queue<PixelCoord> labeling_queue;
    labeling_queue.push(start);
    // while the queue is not empty continue removing front point adding its
    // neighbors back to the queue - breadth-first-search one component
    size_t max_queue_size = 0;
    while (!labeling_queue.empty()) {
      max_queue_size = std::max(labeling_queue.size(), max_queue_size);
      // copy the current coordinate
      const PixelCoord current = labeling_queue.front();
      labeling_queue.pop();
      uint16_t current_label = LabelAt(current);//get the lable of the current pixel value
      if (current_label > 0) {
        // we have already labeled this point. No need to add it.
        continue;
      }
      // set the label of this point to current label
      SetLabel(current, label);

      // check the depth
      auto current_depth = DepthAt(current);//get the depth of the current pixel value
      if (current_depth < 0.001f) { //if the value is too small ,we just consider it is the ground .
        // depth of this point is wrong, so don't bother adding it to queue
        continue;
      }

      for (const auto& step : Neighborhood) {

          PixelCoord neighbor = current + step;
          if (neighbor.row < 0 || neighbor.row >= _label_image.rows) {
            // point doesn't fit
            continue;//如果还是还在这个在图像范围之内的话，还是需要进行处理的，否则的话需要进行标记判别是哪个
          }
          // if we just went over the borders in horiz direction - wrap around
          neighbor.col = WrapCols(neighbor.col);//这里是在处理一些简单的边界情况
          uint16_t neigh_label = LabelAt(neighbor);
          if (neigh_label > 0) {
            // we have already labeled this one//看一下周围邻居的lable
            continue;
          }
          auto diff = diff_helper->DiffAt(current, neighbor);
       //  std::cout<<"no problem on the top ! in the lablecomponent"<<std::endl;
          if (diff_helper->SatisfiesThreshold(diff, _radians_threshold)) {
            labeling_queue.push(neighbor);
          }
      }
    }
  }
  /**
   * @brief      Gets depth value at pixel
   *
   * @param[in]  coord  Pixel coordinate//坐标像素坐标
   *
   * @return     depth at pixel
   */
  inline float DepthAt(const PixelCoord& coord) const {
    return _depth_image_ptr->at<float>(coord.row, coord.col);
  }

  /**
   * @brief      Gets label of a given pixel
   *
   * @param[in]  coord  Pixel coordinate
   *
   * @return     label for pixel
   */
  inline uint16_t LabelAt(const PixelCoord& coord) const {
   // std::cout<<"The value of the _label_image.at<uint16_t>(coord.row, coord.col) is ;"<<coord.col<<std::endl;
    return _label_image.at<uint16_t>(coord.row, coord.col);
  }

  /**
   * @brief      Sets label of a given pixel
   *
   * @param[in]  coord  Pixel coordinate
   */
  inline void SetLabel(const PixelCoord& coord, uint16_t label) {
    _label_image.at<uint16_t>(coord.row, coord.col) = label;
  }

  /**
   * @brief      Wrap columns around image
   */
  int16_t WrapCols(int16_t col) const {
    // we allow our space to fold around cols
    if (col < 0) {
     // std::cout<<" the value of the col is smaller than 0;and the value of it is :"<<col<<std::endl;
    //  std::cout<<" the value of the col + _label_image.colsis :"<<col + _label_image.cols<<std::endl;
      return col + _label_image.cols;
    }
    if (col >= _label_image.cols) {
     // std::cout<<"the value of the cols is larger than the _lable_image.cols,and the value of it is :"<<col<<std::endl;
     // std::cout<<" the value of thecol - _label_image.cols :"<<col - _label_image.cols<<std::endl;
      return col - _label_image.cols;
    }
    //std::cout<<"the value of the col is :"<<col<<std::endl;
    return col; //否则的话就返回当前的列的值　
  }

  /**
   * @brief      Calculates the labels running over the whole image.
   */
  void ComputeLabels(DiffFactory::DiffType diff_type) override {
    _label_image =
        cv::Mat::zeros(_depth_image_ptr->size(), cv::DataType<uint16_t>::type);
    auto diff_helper_ptr =
        DiffFactory::Build(diff_type, _depth_image_ptr, &_params);
     std::cout<<"the build function was found in the liner_image_lable .h"<<std::endl;
    // initialize the label
    uint16_t label = 1;

    for (int row = 0; row < _label_image.rows; ++row) {
      for (int col = 0; col < _label_image.cols; ++col) {
        if (_label_image.at<uint16_t>(row, col) > 0) {
          // we have already labeled this point
          continue;
        }
        if (_depth_image_ptr->at<float>(row, col) < 0.001f) {
          // depth is zero, not interested
          continue;
        }
        LabelOneComponent(label, PixelCoord(row, col), diff_helper_ptr.get());//在这里计算其中的一个连通区域，然后再接着计算其它的连通区域
        // we have finished labeling this connected component. We now need to
        // label the next one, so we increment the label
        label++;
      }
    }
  }
};

}  // namespace depth_clustering

#endif  // SRC_IMAGE_LABELERS_LINEAR_IMAGE_LABELER_H_
