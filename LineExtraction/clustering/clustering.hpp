//
//  clustering.hpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_clustering_hpp
#define LineExtraction_clustering_hpp

#include "Common.hpp"

//// http://stackoverflow.com/questions/13290395/how-to-remove-a-certain-row-or-column-while-using-eigen-library-c
template<typename T>
void removeRow(T& matrix, unsigned int rowToRemove)
{
  unsigned int numRows = matrix.rows()-1;
  unsigned int numCols = matrix.cols();
  
  if( rowToRemove < numRows )
    matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);
  
  matrix.conservativeResize(numRows,numCols);
}

template<typename T>
void removeColumn(T& matrix, unsigned int colToRemove)
{
  unsigned int numRows = matrix.rows();
  unsigned int numCols = matrix.cols()-1;
  
  if( colToRemove < numCols )
    matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);
  matrix.conservativeResize(numRows,numCols);
}
//// --------

template<class DataContainer, class DistanceOperator, class Linkage>
class AgglomerativeHierarchical{
  typedef typename DataContainer::const_iterator                                  t_DataIterator;
  typedef DistanceOperator                                                        t_DistanceOperator;
  typedef typename DistanceOperator::distance_type                                t_DistanceType;
  typedef typename Eigen::Matrix<t_DistanceType,Eigen::Dynamic,Eigen::Dynamic>    t_DistanceMatrix;
  typedef Linkage                                                                 t_Linkage;
  typedef vector<vector<size_t>>                                                  t_IndexedIndexVector;
  typedef array<size_t, 2>                                                        t_MergedIndexes;
  typedef vector<size_t>                                                          t_IndexVector;
  
  size_t                    _cluster_size;
  t_DistanceMatrix          _distance_matrix;
  t_Linkage                 _linkage;
  t_IndexedIndexVector      _merged_objects;
  t_MergedIndexes           _indexes_to_merge;
  t_IndexVector             _merged;
  t_IndexVector             _not_clustered;
  t_IndexedIndexVector      _temp_clustered;
  t_DistanceMatrix          _temp_matrix;
  /// Current clusters
  t_DistanceOperator        _distanceop;
  t_IndexVector             _out_DataIndexes;
  t_DistanceMatrix          _out_DistanceMatrix;
  t_DistanceType            _out_Distance;
  
  void calculateDistances(t_DataIterator first,
                          t_DataIterator last){
    _cluster_size = distance(first,last);
    _distance_matrix.resize(_cluster_size,_cluster_size);
    _distance_matrix.setConstant(_distanceop.infinity());
    
    size_t  ii = 0, jj = 0;
    _merged_objects.resize(_cluster_size);
    
    for ( t_DataIterator i = first; i != last; i++, ii++) {
      for ( t_DataIterator j = first; j != last; j++, jj++){
        if (ii == _cluster_size)
          ii=0;
        if (jj == _cluster_size)
          jj=0;
        if (ii==jj) continue;
        _distance_matrix(ii,jj) = _distanceop(*i,*j);//(*i)-(*j);
        _distance_matrix(jj,ii) = _distance_matrix(ii,jj);
      }
      _merged_objects[ii].push_back(ii);
    }
  }
  
public:
  
  explicit AgglomerativeHierarchical(t_DataIterator first,
                                     t_DataIterator last,
                                     const DistanceOperator& op,
                                     const Linkage& link):
  _linkage(link),
  _distanceop(op),
  AgglomerativeHierarchical(first,last){}
  
  explicit AgglomerativeHierarchical(t_DataIterator first,
                                     t_DataIterator last):
  _linkage(_distanceop.infinity()){
    
    calculateDistances(first,last);
    
    cout << _distance_matrix << endl;
    
    _temp_matrix.resize(_cluster_size,_cluster_size);
  }
  
  void currentClusterIndexes(t_IndexVector&& out_DataIndexes){
    out_DataIndexes = move(_out_DataIndexes);
  }
  
  void currentClusterDistances(t_DistanceMatrix&& out_DistanceMatrix){
    out_DistanceMatrix = move(_out_DistanceMatrix);
  }
  
  void currentClusterDistance(t_DistanceType& out_Distance){
    out_Distance = _out_Distance;
  }
  
  bool nextClusters(){
    if (_cluster_size <= 1)
      return false;
    
    _linkage.Value(_distanceop.infinity());
    
    for (size_t i=0; i < _distance_matrix.cols(); i++) {
      for (size_t j=0; j < _distance_matrix.rows(); j++) {
        if ( i == j ) continue;
        if (_linkage(_distance_matrix(i,j))){
          _indexes_to_merge[0] = i;
          _indexes_to_merge[1] = j;
        }
      }
    }
    
    _merged.clear();
    _not_clustered.clear();
    
    for (int i=0 ; i < _merged_objects[_indexes_to_merge[0]].size(); i++) {
      _merged.push_back(_merged_objects[_indexes_to_merge[0]][i]);
    }
    for (int i=0; i < _merged_objects[_indexes_to_merge[1]].size(); i++) {
      _merged.push_back(_merged_objects[_indexes_to_merge[1]][i]);
    }
    
    for (int i=0; i < _merged_objects.size(); i++) {
      if ( _indexes_to_merge[0] != i && _indexes_to_merge[1] != i ){
        _temp_clustered.push_back(_merged_objects[i]);
        _not_clustered.push_back(i);
      }
    }
    
    _temp_clustered.push_back(_merged);//// CHANGED
    
    _out_DataIndexes = move(_merged);
    ///copy to temp then move
    _out_DistanceMatrix = _distance_matrix;
    _out_Distance = _linkage.Value();
    
    //clustersChanged(merged,distance_matrix,linkage.Value());
    
    _temp_matrix = _distance_matrix;
    
    removeColumn(_temp_matrix, _indexes_to_merge[0]);
    //cout << endl << temp_matrix << endl;
    removeRow(_temp_matrix, _indexes_to_merge[0]);
    //cout << endl << temp_matrix << endl;
    removeColumn(_temp_matrix, _indexes_to_merge[1]-1);
    //cout << endl << temp_matrix << endl;
    
    removeRow(_temp_matrix, _indexes_to_merge[1]-1);
    //cout << endl << temp_matrix << endl;
    _temp_matrix.conservativeResize(_temp_matrix.rows()+1, _temp_matrix.cols()+1);
    
    for (size_t i =0; i < _not_clustered.size(); i++) {
      _linkage.Value(_distanceop.infinity());
      _linkage(_distance_matrix(_not_clustered[i],_indexes_to_merge[0]));
      _linkage(_distance_matrix(_not_clustered[i],_indexes_to_merge[1]));
      _temp_matrix(i,_cluster_size-2) = _linkage.Value();
      _temp_matrix(_cluster_size-2,i) = _linkage.Value();
    }
    
    //cout << endl << temp_matrix << endl;
    _distance_matrix = move(_temp_matrix);
    _merged_objects = _temp_clustered;
    _temp_clustered.clear();
    _cluster_size--;
    return true;
  }
};


template<class DataContainer, class SimilarityMeasure>
class SequentialFuzzyClustering{
  typedef typename DataContainer::const_iterator t_DataIterator;
  typedef typename SimilarityMeasure::distance_type t_Distance;
  
  size_t                        dataSize;
  vector<float_t>       currentMembership;
  SimilarityMeasure similarity;
  Eigen::Matrix<t_Distance,Eigen::Dynamic,Eigen::Dynamic>   similarityMatrix;
  
  bool duplicate(
                 pair<size_t,vector<size_t>> cluster1,
                 pair<size_t,vector<size_t>> cluster2,
                 float_t w){
    return false;
  }
  
public:
  SequentialFuzzyClustering(t_DataIterator first,
                            t_DataIterator last,
                            SimilarityMeasure& sm = SimilarityMeasure()){
    dataSize = distance(first,last);
    currentMembership.resize(dataSize,0.0f);
    similarity = move(sm);
    similarityMatrix.resize(dataSize,dataSize);
    
    size_t ii=0;
    for(t_DataIterator i = first; i != last; i++){
      size_t jj = 0;
      for(t_DataIterator j = first; j != last; j++){
        similarityMatrix(ii,jj) = similarity(first,last);
        jj++;
      }
      ii++;
    }
  }
  
  bool nextClusters(){
    
    
    return true;
  }
  
};

#endif
