/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$ 
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef QmitkMITKIGTTrackingToolboxView_h
#define QmitkMITKIGTTrackingToolboxView_h

#include <berryISelectionListener.h>

#include <QmitkFunctionality.h>

#include "ui_QmitkMITKIGTTrackingToolboxViewControls.h"



/*!
  \brief QmitkMITKIGTTrackingToolboxView 

  \warning  This application module is not yet documented. Use "svn blame/praise/annotate" and ask the author to provide basic documentation.

  \sa QmitkFunctionality
  \ingroup Functionalities
*/
class QmitkMITKIGTTrackingToolboxView : public QObject, public QmitkFunctionality
{  
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT
  
  public:  

    static const std::string VIEW_ID;

    QmitkMITKIGTTrackingToolboxView();
    virtual ~QmitkMITKIGTTrackingToolboxView();

    virtual void CreateQtPartControl(QWidget *parent);

    virtual void StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget);
    virtual void StdMultiWidgetNotAvailable();

  protected slots:
  
    /// \brief Called when the user clicks the GUI button
    void DoImageProcessing();

  protected:

    /// \brief called by QmitkFunctionality when DataManager's selection has changed
    virtual void OnSelectionChanged( std::vector<mitk::DataNode*> nodes );

    Ui::QmitkMITKIGTTrackingToolboxViewControls* m_Controls;

    QmitkStdMultiWidget* m_MultiWidget;
};



#endif // _QMITKMITKIGTTRACKINGTOOLBOXVIEW_H_INCLUDED

