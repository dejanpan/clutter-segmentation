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
 * $Id: nearest_neighbors.cpp 49424 2011-02-23 01:06:38Z aaldoma $
 *
 */

/**
  * \author Radu Bogdan Rusu 
  *
  * @b nearest_neighbors loads a FLANN VFH kd-tree from disk, together with a
  * list of model names, and returns the closest K neighbors to a given query
  * model/VFH signature.
  */

#include <vfh_cluster_classifier/common_io.h>
#include <iostream>
#include <utility>
#include <cctype>
#include <flann/flann.h>
#include <flann/io/hdf5.h>

#include <terminal_tools/parse.h>
#include <terminal_tools/print.h>

#include <pcl/common/common.h>
#include <pcl/features/feature.h>
#include <pcl/registration/transforms.h>
#include <pcl_visualization/pcl_visualizer.h>

using vfh_cluster_classifier::vfh_model;

using namespace std;
using namespace pcl;
using namespace terminal_tools;
using namespace pcl_visualization;

#define SIGMA 2

////////////////////////////////////////////////////////////////////////////////
/** \brief Search for the closest k neighbors
  * \param index the tree
  * \param model the query model
  * \param k the number of neighbors to search for
  * \param indices the resultant neighbor indices
  * \param distances the resultant neighbor distances
  */
inline void
  nearestKSearch (flann::Index<flann::ChiSquareDistance<float> > &index, const vfh_model &model, 
                  int k, flann::Matrix<int> &indices, flann::Matrix<float> &distances)
{
  // Query point
  flann::Matrix<float> p = flann::Matrix<float>(new float[model.second.size ()], 1, model.second.size ());
  memcpy (&p.data[0], &model.second[0], p.cols * p.rows * sizeof (float));

  indices = flann::Matrix<int>(new int[k], 1, k);
  distances = flann::Matrix<float>(new float[k], 1, k);
  index.knnSearch (p, indices, distances, k, flann::SearchParams (512));
  p.free();
}

////////////////////////////////////////////////////////////////////////////////
/** \brief Load the list of file model names from an ASCII file
  * \param models the resultant list of model name
  * \param filename the input file name
  */
bool
  loadFileList (std::vector<vfh_model> &models, const std::string &filename)
{
  ifstream fs;
  fs.open (filename.c_str ());
  if (!fs.is_open () || fs.fail ())
    return (false);

  std::string line;
  while (!fs.eof ())
  {
    getline (fs, line);
    if (line.empty ())
      continue;
    vfh_model m;
    m.first = line;
    models.push_back (m);
  }
  fs.close ();
  return (true);
}

double
  computeThreshold (const flann::Matrix<float> &data, int k, double sigma)
{
  vector<float> distances (k);
  for (int i = 0; i < k; ++i)
    distances[i] = data[0][i];

  double mean, stddev;
  pcl::getMeanStdDev (distances, mean, stddev);

  // Outlier rejection
  vector<float> inliers;
  for (int i = 0; i < k; ++i)
    if ((distances[i] <= (mean + sigma * stddev)) && (distances[i] >= (mean - sigma * stddev)))
      inliers.push_back (distances[i]);

  pcl::getMeanStdDev (inliers, mean, stddev);
  
  return (mean);
}

/* ---[ */
int
  main (int argc, char** argv)
{
  int k = 6;
  int metric = 7;

  double thresh = DBL_MAX;     // No threshold, disabled by default

  if (argc < 2)
  {
    print_error ("Need at least three parameters! Syntax is: %s <query_vfh_model.pcd> [options] {kdtree.idx} {training_data.h5} {training_data.list}\n", argv[0]);
    print_info ("    where [options] are:  -k      = number of nearest neighbors to search for in the tree (default: "); print_value ("%d", k); print_info (")\n");
    print_info ("                          -metric = metric/distance type:  1 = Euclidean, 2 = Manhattan, 3 = Minkowski, 4 = Max, 5 = HIK, 6 = JM, 7 = Chi-Square (default: "); print_value ("%d", metric); print_info (")\n\n");
    print_info ("                          -thresh = maximum distance threshold for a model to be considered VALID (default: "); print_value ("%f", thresh); print_info (")\n\n");

    //
    print_info ("      * note: the metric_type has to match the metric that was used when the tree was created.\n");
    print_info ("              the last three parameters are optional and represent: the kdtree index file (default: "); print_value ("kdtree.idx"); print_info (")\n"
                "                                                                    the training data used to create the tree (default: "); print_value ("training_data.h5"); print_info (")\n"
                "                                                                    the list of models used in the training data (default: "); print_value ("training_data.list"); print_info (")\n");
    return (-1);
  }

  //string extension (".VpFH");
  string extension (".pcd");
  transform (extension.begin (), extension.end (), extension.begin (), (int(*)(int))tolower);

  // Load the test histogram
  vector<int> pcd_indices = parse_file_extension_argument (argc, argv, ".pcd");
  if (pcd_indices.size () > 1)
  {
    print_error ("Need a single test file!\n");
    return (-1);
  }
  vfh_model histogram;
  if (!vfh_cluster_classifier::loadHist (argv[pcd_indices.at (0)], histogram))
  {
    print_error ("Cannot load test file %s\n", argv[pcd_indices.at (0)]);
    return (-1);
  }

  parse_argument (argc, argv, "-thresh", thresh);
  // Search for the k closest matches
  parse_argument (argc, argv, "-k", k);
  print_highlight ("Using "); print_value ("%d", k); print_info (" nearest neighbors.\n");
  // Set the tree metric type
  parse_argument (argc, argv, "-metric", metric);
  if (metric < 0 || metric > 7)
  {
    print_error ("Invalid metric specified (%d)!\n", metric);
    return (-1);
  }
  //flann_set_distance_type ((flann_distance_t)metric, 0);
  print_highlight ("Using distance metric = "); print_value ("%d\n", metric); 

  // --[ Read the kdtree index file
  string kdtree_idx_file_name = "kdtree.idx";
  vector<int> idx_indices = parse_file_extension_argument (argc, argv, ".idx");
  if (idx_indices.size () > 1)
  {
    print_error ("Need a single kdtree index file!\n");
    return (-1);
  }
  if (idx_indices.size () == 1)
    kdtree_idx_file_name = argv[idx_indices.at (0)];
  print_highlight ("Using "); print_value ("%s", kdtree_idx_file_name.c_str ()); print_info (" as the kdtree index file.\n");

  // --[ Read the training data h5 file
  string training_data_h5_file_name = "training_data.h5";
  vector<int> train_h5_indices = parse_file_extension_argument (argc, argv, ".h5");
  if (train_h5_indices.size () > 1)
  {
    print_error ("Need a single h5 training data file!\n");
    return (-1);
  }
  if (train_h5_indices.size () == 1)
    training_data_h5_file_name = argv[train_h5_indices.at (0)];
  print_highlight ("Using "); print_value ("%s", training_data_h5_file_name.c_str ()); print_info (" as the h5 training data file.\n");

  // --[ Read the training data list file
  string training_data_list_file_name = "training_data.list";
  vector<int> train_list_indices = parse_file_extension_argument (argc, argv, ".list");
  if (train_list_indices.size () > 1)
  {
    print_error ("Need a single list training data file!\n");
    return (-1);
  }
  if (train_list_indices.size () == 1)
    training_data_list_file_name = argv[train_list_indices.at (0)];
  print_highlight ("Using "); print_value ("%s", training_data_list_file_name.c_str ()); print_info (" as the list training data file.\n");


  vector<vfh_model> models;
  flann::Matrix<int> k_indices;
  flann::Matrix<float> k_distances;
  flann::Matrix<float> data;
  // Check if the data has already been saved to disk
  if (!boost::filesystem::exists ("training_data.h5") || !boost::filesystem::exists ("training_data.list"))
  {
    print_error ("Could not find training data models files %s and %s!\n", training_data_h5_file_name.c_str (), training_data_list_file_name.c_str ());
    return (-1);
  }
  else
  {
    loadFileList (models, training_data_list_file_name);
    flann::load_from_file (data, training_data_h5_file_name, "training_data");
    print_highlight ("Training data found. Loaded %d VFH models from %s/%s.\n", (int)data.rows, training_data_h5_file_name.c_str (), training_data_list_file_name.c_str ());
  }

  // Check if the tree index has already been saved to disk
  if (!boost::filesystem::exists (kdtree_idx_file_name))
  {
    print_error ("Could not find kd-tree index in file %s!", kdtree_idx_file_name.c_str ());
    return (-1);
  }
  else
  {
    flann::Index<flann::ChiSquareDistance<float> > index (data, flann::SavedIndexParams ("kdtree.idx"));
    index.buildIndex ();
    nearestKSearch (index, histogram, k, k_indices, k_distances);
  }

  // Output the results on screen
  print_highlight ("The closest %d neighbors for %s are:\n", k, argv[pcd_indices[0]]);
  for (int i = 0; i < k; ++i)
    print_info ("    %d - %s (%d) with a distance of: %f\n", i, models.at (k_indices[0][i]).first.c_str (), k_indices[0][i], k_distances[0][i]);

  // Compute the optimal separation threshold if 'thresh' not given
  if (thresh == DBL_MAX || thresh < 0)
    thresh = computeThreshold (k_distances, k, SIGMA);
  print_highlight ("Using "); print_value ("%f", thresh); print_info (" as an inlier distance threshold.\n");

  // Load the results
  PCLVisualizer p (argc, argv, "VFH Cluster Classifier");
  int y_s = (int)floor (sqrt (k));
  int x_s = y_s + (int)ceil ((k / (double)y_s) - y_s);
  double x_step = (double)(1 / (double)x_s);
  double y_step = (double)(1 / (double)y_s);
  print_highlight ("Preparing to load "); print_value ("%d", k); print_info (" files ("); 
  print_value ("%d", x_s);    print_info ("x"); print_value ("%d", y_s); print_info (" / ");
  print_value ("%f", x_step); print_info ("x"); print_value ("%f", y_step); print_info (")\n");

  int viewport = 0, l = 0, m = 0;
  for (int i = 0; i < k; ++i)
  {
    string cloud_name = models.at (k_indices[0][i]).first;
    boost::replace_last (cloud_name, "_vfh", "");

    p.createViewPort (l * x_step, m * y_step, (l + 1) * x_step, (m + 1) * y_step, viewport);
    l++;
    if (l >= x_s)
    {
      l = 0;
      m++;
    }

    sensor_msgs::PointCloud2 cloud;
    print_highlight (stderr, "Loading "); print_value (stderr, "%s ", cloud_name.c_str ());
    if (pcl::io::loadPCDFile (cloud_name, cloud) == -1)
      break;

    // Convert from blob to PointCloud
    PointCloud<PointXYZ> cloud_xyz;
    fromROSMsg (cloud, cloud_xyz);

    if (cloud_xyz.points.size () == 0)
      break;

    print_info ("[done, "); print_value ("%d", (int)cloud_xyz.points.size ()); print_info (" points]\n");
    print_info ("Available dimensions: "); print_value ("%s\n", getFieldsList (cloud).c_str ());

    // Demean the cloud
    Eigen::Vector4f centroid;
    compute3DCentroid (cloud_xyz, centroid);
    PointCloud<PointXYZ> cloud_xyz_demean;
    demeanPointCloud<PointXYZ> (cloud_xyz, centroid, cloud_xyz_demean);
    // Add to renderer
    p.addPointCloud (cloud_xyz_demean, cloud_name, viewport);
    
    // Check if the model found is within our inlier tolerance
    stringstream ss;
    ss << k_distances[0][i];
    if (k_distances[0][i] > thresh)
    {
      p.addText (ss.str (), 20, 30, 1, 0, 0, ss.str (), viewport);  // display the text with red

      // Create a red line
      PointXYZ min_p, max_p;
      pcl::getMinMax3D (cloud_xyz_demean, min_p, max_p);
      stringstream line_name;
      line_name << "line_" << i;
      p.addLine (min_p, max_p, 1, 0, 0, line_name.str (), viewport);
      p.setShapeRenderingProperties (PCL_VISUALIZER_LINE_WIDTH, 5, line_name.str (), viewport);
    }
    else
      p.addText (ss.str (), 20, 30, 0, 1, 0, ss.str (), viewport);

    // Increase the font size for the score
    p.setShapeRenderingProperties (PCL_VISUALIZER_FONT_SIZE, 18, ss.str (), viewport);

    // Add the cluster name
    p.addText (cloud_name, 20, 10, cloud_name, viewport);
  }
  // Add coordianate systems to all viewports
  p.addCoordinateSystem (0.1, 0);
  //p.setBackgroundColor (1.0, 1.0, 1.0);

  p.spin ();
  return (0);
}
/* ]--- */
