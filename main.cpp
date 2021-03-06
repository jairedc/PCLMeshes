#include <iostream>
#include <ostream>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/io/ply_io.h>
#include <pcl/features/normal_3d.h>
#include <pcl/search/kdtree.h>
#include <pcl/surface/gp3.h>

typedef pcl::PointCloud<pcl::PointXYZ> PCXYZ;
typedef pcl::PointCloud<pcl::Normal> PCNormal;
typedef pcl::PointCloud<pcl::PointNormal> PCPointNormal;
typedef pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> NormalEstimation;
typedef pcl::search::KdTree<pcl::PointXYZ> KdTree;
typedef pcl::search::KdTree<pcl::PointNormal> KdTreeNormals;
typedef pcl::GreedyProjectionTriangulation<pcl::PointNormal> GPTriangle;

int main(int argc, char** argv)
{
  std::string plyFilename = "/home/jaired/Datasets/PointClouds/ABQ-215-1m-Meru3.ply";
  double searchRadius = 3;
  if (argc == 2)
    plyFilename = std::string(argv[1]);
  if (argc == 3)
    searchRadius = std::stod(argv[2]);

  PCXYZ::Ptr cloud(new PCXYZ);

  std::cout << "Attempting to read file at " << plyFilename << "...";
  int rc = pcl::io::loadPLYFile(plyFilename, *cloud);
  if (rc)
  {
    std::cout << "Can't read PLY file." << std::endl;
    return 1;
  }
  std::cout << "done. File successfully read." << std::endl;

  // Normals
  std::cout << "Estimating normals... ";
  NormalEstimation ne;
  PCNormal::Ptr normals(new PCNormal);
  KdTree::Ptr tree(new KdTree());
  tree->setInputCloud(cloud);
  ne.setInputCloud(cloud);
  ne.setSearchMethod(tree);
  ne.setRadiusSearch(searchRadius);
  ne.compute(*normals);
  std::cout << "done. " << std::endl;
  std::cout << "Point cloud count: " << cloud->size() << std::endl;
  std::cout << "Normal count: " << normals->size() << std::endl;
  if (cloud->size() != normals->size())
  {
    std::cout << "Counts don't match. Exiting..." << std::endl;
    return 2;
  }

  // Concatenate XYZ and normals
  PCPointNormal::Ptr cloud_with_normals(new PCPointNormal);
  pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

  // Search tree
  KdTreeNormals::Ptr tree2(new KdTreeNormals);
  tree2->setInputCloud(cloud_with_normals);

  // Initialize objects
  GPTriangle gp3;
  pcl::PolygonMesh triangles;

  gp3.setSearchRadius(searchRadius);

  // Set "typical" values for parameters
  gp3.setMu(2.5);
  gp3.setMaximumNearestNeighbors (100);
  gp3.setMaximumSurfaceAngle(M_PI/4); // 45 degrees
  gp3.setMinimumAngle(M_PI/18); // 10 degrees
  gp3.setMaximumAngle(2*M_PI/3); // 120 degrees
  gp3.setNormalConsistency(false);

  // Get result
  std::cout << "Creating mesh...";
  gp3.setInputCloud(cloud_with_normals);
  gp3.setSearchMethod(tree2);
  gp3.reconstruct(triangles);
  std::cout << "done." << std::endl;

  pcl::io::savePLYFile("mesh.ply", triangles);

  return 0;
}
