/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: common_io.h 49424 2011-02-23 01:06:38Z aaldoma $
 *
 */

#ifndef VFH_CLUSTER_CLASSIFIER_COMMON_IO_H_
#define VFH_CLUSTER_CLASSIFIER_COMMON_IO_H_

#include <ros/message_traits.h>
#include <ros/serialization.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/io/pcd_io.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <pcl_ros/point_cloud.h>
#include <flann/flann.h>
#include <flann/io/hdf5.h>
#include <fstream>
#include <iostream>

namespace ser = ros::serialization;

namespace vfh_cluster_classifier
{
  typedef std::pair<std::string, std::vector<float> > vfh_model;

  ////////////////////////////////////////////////////////////////////////////////
  /** \brief Loads an n-D histogram file as a VFH signature
   * @note: VFH histogram files have no header, so they can be used with gnuplot
   * \param file_name the input file name
   * \param vfh the resultant VFH model
   */
  inline bool
  loadHist (const boost::filesystem::path &path, vfh_model &vfh)
  {
    using namespace std;
    using namespace pcl;

    int vfh_idx;
    // Load the file as a PCD
    stringstream ss;
    ss << path;
    try
    {
      sensor_msgs::PointCloud2 cloud;
      int version;
      Eigen::Vector4f origin;
      Eigen::Quaternionf orientation;
      PCDReader r;
      r.readHeader (ss.str (), cloud, origin, orientation, version);

      vfh_idx = getFieldIndex (cloud, "vfh");
      if (vfh_idx == -1)
        return (false);
      if ((int)cloud.width * cloud.height != 1)
        return (false);
    }
    catch (pcl::InvalidConversionException e)
    {
      return (false);
    }

    // Treat the VFH signature as a single Point Cloud
    PointCloud < VFHSignature308 > point;
    io::loadPCDFile (ss.str (), point);
    vfh.second.resize (308);

    vector < sensor_msgs::PointField > fields;
    getFieldIndex (point, "vfh", fields);

    for (size_t i = 0; i < fields[vfh_idx].count; ++i)
    {
      vfh.second[i] = point.points[0].histogram[i];
    }
    vfh.first = ss.str ();
    return (true);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /** \brief Loads an n-D histogram file as a VFH signature
   * @note: VFH histogram files have no header, so they can be used with gnuplot
   * \param file_name the input file name
   * \param vfh the resultant VFH model
   */
  inline bool
  loadHistPlain (const boost::filesystem::path &path, vfh_model &vfh)
  {
    std::ifstream fs;
    std::string line, file_name;
    std::stringstream ss;
    ss << path;
    file_name = ss.str ();

    // Open file
    fs.open (file_name.c_str ());
    if (!fs.is_open () || fs.fail ())
    {
      fprintf (stderr, "Couldn't open %s for reading!\n", file_name.c_str ());
      return (false);
    }

    // Get the number of lines (points) this histogram file holds
    int nr_samples = 0;
    while ((!fs.eof ()) && (getline (fs, line)))
      nr_samples++;

    vfh.second.resize (nr_samples);

    fs.clear ();
    fs.seekg (0, std::ios_base::beg);
    for (int i = 0; i < nr_samples; ++i)
    {
      getline (fs, line);
      if (line == "")
        continue;

      vfh.second[i] = atof (line.c_str ());
    }
    vfh.first = file_name;
    // Close file
    fs.close ();
    return (true);
  }
}
#endif // VFH_CLUSTER_CLASSIFIER_COMMON_IO_H_
