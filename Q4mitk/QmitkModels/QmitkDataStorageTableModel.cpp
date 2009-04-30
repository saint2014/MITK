#include "QmitkDataStorageTableModel.h"

#include "mitkNodePredicateBase.h"
#include "mitkProperties.h"
#include "mitkRenderingManager.h"

#include "itkCommand.h"

#include <QIcon>

QmitkDataStorageTableModel::QmitkDataStorageTableModel(mitk::DataStorage::Pointer _DataStorage
                                                       , mitk::NodePredicateBase* _Predicate
                                                       , QObject* parent )
: QAbstractTableModel(parent)
, m_DataStorage(0)
, m_NodeSet(mitk::DataStorage::SetOfObjects::New())
, m_BlockEvents(false)
{
  m_Predicate = _Predicate;
  this->setDataStorage(_DataStorage);
}

void QmitkDataStorageTableModel::reset()
{
  mitk::DataStorage::SetOfObjects::ConstPointer _NodeSet;

  // remove all event listeners

  // remove all event listeners
  for(std::map<mitk::BaseProperty*, unsigned long>::iterator it=m_PropertyModifiedObserverTags.begin()
    ; it!=m_PropertyModifiedObserverTags.end(); it++)
  {
    it->first->RemoveObserver(it->second);
  }

  m_PropertyModifiedObserverTags.clear();
  m_NodeSet->clear();

  // the whole reset depends on the fact if a data storage is set or not
  if(m_DataStorage != 0)
  {

    if(m_Predicate.IsNotNull())
      // get subset
      _NodeSet = m_DataStorage->GetSubset(m_Predicate);
    // if predicate is NULL, select all nodes
    else
    {
      _NodeSet = m_DataStorage->GetAll();
      // remove ghost root node
    }

    // only add nodes with appended data (no one wants to see an empty node)
    for(mitk::DataStorage::SetOfObjects::const_iterator it=_NodeSet->begin(); it!=_NodeSet->end()
      ; it++)
    {      
      if((*it)->GetData() != 0)  
      {
        m_NodeSet->push_back(*it);
        itk::MemberCommand<QmitkDataStorageTableModel>::Pointer propertyModifiedCommand =
          itk::MemberCommand<QmitkDataStorageTableModel>::New();
        propertyModifiedCommand->SetCallbackFunction(this, &QmitkDataStorageTableModel::PropertyModified);

        mitk::BaseProperty* visibilityProperty = (*it)->GetProperty("visible");
        if(visibilityProperty)
          m_PropertyModifiedObserverTags[visibilityProperty] 
        = visibilityProperty->AddObserver(itk::ModifiedEvent(), propertyModifiedCommand);

        mitk::BaseProperty* nameProperty = (*it)->GetProperty("name");
        if(nameProperty)
          m_PropertyModifiedObserverTags[nameProperty] 
        = nameProperty->AddObserver(itk::ModifiedEvent(), propertyModifiedCommand);
      }
    }
  }
    
  // emit reset signals
  QAbstractItemModel::reset();
}

QmitkDataStorageTableModel::~QmitkDataStorageTableModel()
{
  // set data storage 0 to remove event listeners
  this->setDataStorage(0);
}

void QmitkDataStorageTableModel::setPredicate(mitk::NodePredicateBase* _Predicate)
{
  // ensure that a new predicate is set in order to avoid unnecessary changed events
  if(m_Predicate != _Predicate)
  {
    m_Predicate = _Predicate;
    this->reset();
  }
}

mitk::NodePredicateBase* QmitkDataStorageTableModel::getPredicate() const
{
  return m_Predicate;
}

void QmitkDataStorageTableModel::setDataStorage(mitk::DataStorage::Pointer _DataStorage)
{
  // only proceed if we have a new datastorage
  if(m_DataStorage.GetPointer() != _DataStorage.GetPointer())
  {
    // if a data storage was set before remove old event listeners
    if(m_DataStorage != 0)
    {
      this->m_DataStorage->AddNodeEvent.RemoveListener( mitk::MessageDelegate1<QmitkDataStorageTableModel
        , const mitk::DataTreeNode*>( this, &QmitkDataStorageTableModel::NodeAdded ) );

      this->m_DataStorage->RemoveNodeEvent.RemoveListener( mitk::MessageDelegate1<QmitkDataStorageTableModel
        , const mitk::DataTreeNode*>( this, &QmitkDataStorageTableModel::NodeRemoved ) );
    }

    // set new data storage
    m_DataStorage = _DataStorage.GetPointer();

    // if new storage is not 0 subscribe for events
    if(m_DataStorage != 0)
    {
      // subscribe for node added/removed events
      this->m_DataStorage->AddNodeEvent.AddListener( mitk::MessageDelegate1<QmitkDataStorageTableModel
        , const mitk::DataTreeNode*>( this, &QmitkDataStorageTableModel::NodeAdded ) );

      this->m_DataStorage->RemoveNodeEvent.AddListener( mitk::MessageDelegate1<QmitkDataStorageTableModel
        , const mitk::DataTreeNode*>( this, &QmitkDataStorageTableModel::NodeRemoved ) );
    }

    // reset model (even if datastorage is 0->will be checked in reset())
    this->reset();
  }
}

const mitk::DataStorage::Pointer QmitkDataStorageTableModel::getDataStorage() const
{
  return m_DataStorage.GetPointer();
}

int QmitkDataStorageTableModel::rowCount(const QModelIndex &parent) const
{
  return m_NodeSet->size();
}

int QmitkDataStorageTableModel::columnCount(const QModelIndex &parent) const
{
  // show name, type and visible column
  int columns = 3;
  return columns;
}

QVariant QmitkDataStorageTableModel::data(const QModelIndex &index, int role) const
{
  QVariant data;

  if (index.isValid() && !m_NodeSet->empty())
  {
    mitk::DataTreeNode::Pointer node = m_NodeSet->ElementAt(index.row());

    std::string nodeName = node->GetName();
    if(nodeName.empty())
      nodeName = "unnamed";

    // get name
    if(index.column() == 0)
    {
      // get name of node (may also be edited)
      if (role == Qt::DisplayRole || role == Qt::EditRole) 
      {
        data = nodeName.c_str();
      }
    }
    else if (index.column() == 1)
    {
      // Get DataType
      mitk::BaseData* mitkData = node->GetData();
      std::string className;
      if(mitkData)
      {
        className = node->GetData()->GetNameOfClass();
        bool isSegmentation = false;
        node->GetBoolProperty("segmentation", isSegmentation);
        // if we have a binary image, call it image mask
        if(className == "Image" && isSegmentation)
          className = "Image Mask";
      }
      else
        className = "unknown";

      // get type property of mitk::BaseData
      if (role == Qt::DisplayRole) 
      {

        data = QString::fromStdString(className);
      }
      // show some nice icons for datatype
      else if(role == Qt::DecorationRole)
      {
        if(className == "Image")
          data = QIcon(":/datamanager/data-type-image-24.png");
        else if(className == "Image Mask")
          data = QIcon(":/datamanager/data-type-image-mask-24.png");    
        else if(className == "Surface")
          data = QIcon(":/datamanager/data-type-mesh-24.png");
        else if(className == "PointSet")
          data = QIcon(":/datamanager/data-type-pointset-24.png");
        else if(className == "Geometry2DData" && nodeName == "widget1Plane")
          data = QIcon(":/datamanager/data-type-planegeometry1-24.png");
        else if(className == "Geometry2DData" && nodeName == "widget2Plane")
          data = QIcon(":/datamanager/data-type-planegeometry2-24.png");
        else if(className == "Geometry2DData" && nodeName == "widget3Plane")
          data = QIcon(":/datamanager/data-type-planegeometry3-24.png");
        else
          data = QIcon(":/datamanager/data-type-unknown-24.png");
      }
    }
    else if (index.column() == 2)
    {
      // get visible property of mitk::BaseData
      bool visibility = false;
      
      if(node->GetVisibility(visibility, 0))
      {    
        if (role == Qt::DisplayRole || role == Qt::EditRole) 
        {
          data = visibility;
        } // role == Qt::DisplayRole
          
      } // node->GetVisibility(visibility, 0)   

    } // index.column() == 2 

  } // index.isValid() && !m_NodeSet->empty()
  return data;
}

mitk::DataTreeNode::Pointer QmitkDataStorageTableModel::getNode(const QModelIndex &index) const
{
  mitk::DataTreeNode::Pointer node;

  if(index.isValid())
  {
    node = m_NodeSet->ElementAt(index.row());
  }

  return node;
}

QVariant QmitkDataStorageTableModel::headerData(int section, Qt::Orientation orientation,
  int role) const
{
  QVariant headerData;

  // show only horizontal header
  if ( role == Qt::DisplayRole )
  {
    if( orientation == Qt::Horizontal )
    {
      // first column: "Name"
      if(section == 0)
        headerData = "Name";
      else if(section == 1)
        headerData = "Data Type";
      else if(section == 2)
        headerData = "Visibility";
    }
    else if( orientation == Qt::Vertical )
    {
      // show numbers for rows
      headerData = section+1;
    }
  }

  return headerData;
}

bool QmitkDataStorageTableModel::setData(const QModelIndex &index, const QVariant &value,
  int role)
{

  if (index.isValid() && role == Qt::EditRole) 
  {
    // any change events produced here should not be caught in this class
    // --> set m_BlockEvents to true
    m_BlockEvents = true;

    mitk::DataTreeNode::Pointer node = m_NodeSet->ElementAt(index.row());

    if(index.column() == 0)
    {
      node->SetStringProperty("name", value.toString().toStdString().c_str());
    }
    else if(index.column() == 2)
    {
      node->SetBoolProperty("visible", value.toBool());
      mitk::RenderingManager::GetInstance()->RequestUpdateAll();
    }

    // inform listeners about changes
    emit dataChanged(index, index);

    m_BlockEvents = false;
    return true;
  }
  return false;
}

Qt::ItemFlags QmitkDataStorageTableModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags flags = QAbstractItemModel::flags(index);

  // name & visibility is editable
  if (index.column() == 0 || index.column() == 2)
  {
    flags |= Qt::ItemIsEditable;
  }

  return flags;
}

void QmitkDataStorageTableModel::NodeAdded( const mitk::DataTreeNode* node )
{
  // garantuee no recursions when a new node event is thrown
  if(!m_BlockEvents)
  {
    m_BlockEvents = true;

    // check if node should be added to the model
    bool addNode = true;
    if(m_Predicate && !m_Predicate->CheckNode(node))
      addNode = false;

    // currently we dont have the possibility to add single nodes to the node set
    if(addNode)
      this->reset();

    m_BlockEvents = false;
  }
}

void QmitkDataStorageTableModel::NodeRemoved( const mitk::DataTreeNode* node )
{
  // garantuee no recursions when a new node event is thrown
  if(!m_BlockEvents)
  {
    m_BlockEvents = true;

    bool removeNode = false;    
    // check if node is contained in current list, if yes: reset model
    for (mitk::DataStorage::SetOfObjects::ConstIterator nodeIt = m_NodeSet->Begin()
      ; nodeIt != m_NodeSet->End(); nodeIt++)  // for each node
    {
      if( nodeIt.Value() == node )
      {
        removeNode = true;
        break;
      }
    }

    if(removeNode)
      this->reset();

    m_BlockEvents = false;
  }
}

void QmitkDataStorageTableModel::PropertyModified( const itk::Object *caller, const itk::EventObject &event )
{
  if(!m_BlockEvents)
  {
    // get modified property
    const mitk::BaseProperty* modifiedProperty = dynamic_cast<const mitk::BaseProperty*>(caller);

    if(modifiedProperty)
    {
      // find node that holds the modified property
      int row = -1;
      int column = -1;

      mitk::DataStorage::SetOfObjects::iterator it;
      mitk::BaseProperty* visibilityProperty = 0; 
      mitk::BaseProperty* nameProperty = 0;

      // search for property that changed and emit datachanged on the corresponding ModelIndex
      for(it=m_NodeSet->begin(); it!=m_NodeSet->end(); it++)
      {
        // check for the visible property or the name property
        visibilityProperty = (*it)->GetProperty("visible");
        if(modifiedProperty == visibilityProperty)
        {
          column = 2;
          break;
        }

        nameProperty = (*it)->GetProperty("name");
        if(modifiedProperty == nameProperty)
        {
          column = 0;
          break;
        }
      }

      if( it != m_NodeSet->end() )
        row = std::distance(m_NodeSet->begin(), it);

      QModelIndex indexOfChangedProperty = index(row, column);
      emit dataChanged(indexOfChangedProperty, indexOfChangedProperty);
    }
  }
}


/*
void QmitkDataStorageTableModel::NodeChanged( const mitk::DataTreeNode* node )
{
  if(!m_BlockEvents)
  {
    m_BlockEvents = true;

    bool nodeFound = false;    
    // check if node is contained in current list
    mitk::DataStorage::SetOfObjects::iterator nodeIt = 
      std::find(m_NodeSet->begin(), m_NodeSet->end(), node);

    int row = -1;

    // get right index
    if( nodeIt != m_NodeSet->end() )
      row = std::distance( m_NodeSet->begin(), nodeIt );

    // create two model indexes: one beginning at the "name" column
    // and a second one ending at the last column
    if(row >= 0)
    {
      QModelIndex indexOfChangedPropertyFirstColumn = index(row, 0);
      QModelIndex indexOfChangedPropertyLastColumn = index(row, 2);
      emit dataChanged(indexOfChangedPropertyFirstColumn, indexOfChangedPropertyLastColumn);
    }

    m_BlockEvents = false;
  }
}
*/
