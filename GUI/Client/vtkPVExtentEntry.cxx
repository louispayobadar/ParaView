/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtentEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVMinMax.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentEntry);
vtkCxxRevisionMacro(vtkPVExtentEntry, "1.40");

vtkCxxSetObjectMacro(vtkPVExtentEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
vtkPVExtentEntry::vtkPVExtentEntry()
{
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);

  this->Label = 0;

  for (int i=0; i<3; i++)
    {
    this->MinMax[i] = vtkPVMinMax::New();
    }

  this->InputMenu = 0;

  this->Range[0] = this->Range[2] = this->Range[4] = -VTK_LARGE_INTEGER;
  this->Range[1] = this->Range[3] = this->Range[5] = VTK_LARGE_INTEGER;

  this->AnimationAxis = 0;
  this->UseCellExtent = 0;
}

//-----------------------------------------------------------------------------
vtkPVExtentEntry::~vtkPVExtentEntry()
{
  this->LabeledFrame->Delete();
  this->LabeledFrame = 0;

  this->SetLabel(0);

  if (this->InputMenu)
    {
    this->InputMenu->Delete();
    }

  for(int i=0; i<3; i++)
    {
    this->MinMax[i]->Delete();
    this->MinMax[i] = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Update()
{
  this->Superclass::Update();

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMExtentDomain *dom = 0;
  
  if (prop)
    {
    dom = vtkSMExtentDomain::SafeDownCast(prop->GetDomain("extent"));
    }
  
  if (!prop || !dom)
    {
    this->SetRange(0, 0, 0, 0, 0, 0);
    this->SetValue(0, 0, 0, 0, 0, 0);
    }
  else
    {
    int ext[6], i, exists;
    for (i = 0; i < 3; i++)
      {
      ext[2*i] = dom->GetMinimum(i, exists);
      if (!exists)
        {
        ext[2*i] = 0;
        }
      ext[2*i+1] = dom->GetMaximum(i, exists);
      if (!exists)
        {
        ext[2*i+1] = 0;
        }
      }
    
    if (!this->UseCellExtent)
      {
      this->SetRange(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      this->SetValue(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else
      {
      this->SetRange(ext[0], ext[1]-1, ext[2], ext[3]-1, ext[4], ext[5]-1);
      this->SetValue(ext[0], ext[1]-1, ext[2], ext[3]-1, ext[4], ext[5]-1);
      }
    }
}

void vtkPVExtentEntry::SetBalloonHelpString( const char *str )
{
  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
    {
    this->LabeledFrame->SetBalloonHelpString(this->BalloonHelpString);
    for (int i=0; i<3; i++)
      {
      this->MinMax[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // For getting the widget in a script.

  if (this->Label && this->Label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->Label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->LabeledFrame->Create(pvApp, 0);

  // Now a label

  if (this->Label && this->Label[0] != '\0')
    {
    this->LabeledFrame->SetLabel(this->Label);
    }
  else
    {
    this->LabeledFrame->SetLabel("Extent");
    }
   
  char labels[3][4] = { "I: ", "J: ", "K: "};
  int i;
  for(i=0; i<3; i++)
    {
    this->MinMax[i]->SetParent(this->LabeledFrame->GetFrame());
    this->MinMax[i]->PackVerticallyOff();
    this->MinMax[i]->ShowMaxLabelOff();
    this->MinMax[i]->SetMinLabelWidth(2);
    this->MinMax[i]->Create(pvApp);
    this->MinMax[i]->SetRange(this->Range[i*2], this->Range[i*2+1]);
    this->MinMax[i]->SetMinimumLabel(labels[i]);
    this->MinMax[i]->GetMinScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetEntryCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMinScale()->SetDisplayEntryAndLabelOnTop(1);
    this->MinMax[i]->GetMinScale()->DisplayEntry();
    this->MinMax[i]->GetMinScale()->DisplayLabel(" Min.");
    this->MinMax[i]->GetMaxScale()->SetEndCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetEntryCommand(this, "ModifiedCallback");
    this->MinMax[i]->GetMaxScale()->SetDisplayEntryAndLabelOnTop(1);
    this->MinMax[i]->GetMaxScale()->DisplayEntry();
    this->MinMax[i]->GetMaxScale()->DisplayLabel(" Max.");
    }
  
  for(i=0; i<3; i++)
    {
    this->Script("pack %s -side top -fill x -expand t -pady 5", 
                 this->MinMax[i]->GetWidgetName());
    }

  this->Script("pack %s -side left -fill x -expand t", 
               this->LabeledFrame->GetWidgetName());

  this->SetBalloonHelpString(this->BalloonHelpString);
}


//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  int cc;
  for (cc = 0; cc < 3; cc++)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement " << 2*cc << " "
          << this->MinMax[cc]->GetMinValue() << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement " << 2*cc+1 << " "
          << this->MinMax[cc]->GetMaxValue() << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Accept()
{
  int modFlag = this->GetModifiedFlag();
  int i;
  
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (ivp)
    {
    ivp->SetNumberOfElements(6);
    for (i = 0; i < 3; i++)
      {
      ivp->SetElement(2*i, this->MinMax[i]->GetMinValue());
      ivp->SetElement(2*i+1, this->MinMax[i]->GetMaxValue());
      }
    }
  
  this->ModifiedFlag = 0;
  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue ";
  for(int i=0; i<3; i++)
    {
    *file << this->MinMax[i]->GetMinValue() << " "
          << this->MinMax[i]->GetMaxValue() << " ";
    }
  *file << endl;
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  if (this->AcceptCalled)
    {
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetSMProperty());
    if (ivp)
      {
      this->SetValue(ivp->GetElement(0), ivp->GetElement(1),
                     ivp->GetElement(2), ivp->GetElement(3),
                     ivp->GetElement(4), ivp->GetElement(5));
      }
  
    this->ModifiedFlag = 0;
    }
  else
    {
    this->Update();
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetRange(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  this->Range[0] = v0;
  this->Range[1] = v1;
  this->Range[2] = v2;
  this->Range[3] = v3;
  this->Range[4] = v4;
  this->Range[5] = v5;

  if (this->GetApplication())
    {
    this->MinMax[0]->SetRange(v0, v1);
    this->MinMax[1]->SetRange(v2, v3);
    this->MinMax[2]->SetRange(v4, v5);
    }

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::SetValue(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  float range[2];

  // First, restrict value to current range.
  this->MinMax[0]->GetRange(range);
  if (v0 < range[0]) {v0 = static_cast<int>(range[0]);}
  if (v0 > range[1]) {v0 = static_cast<int>(range[1]);}
  if (v1 < range[0]) {v1 = static_cast<int>(range[0]);}
  if (v1 > range[1]) {v1 = static_cast<int>(range[1]);}
  this->MinMax[1]->GetRange(range);
  if (v2 < range[0]) {v2 = static_cast<int>(range[0]);}
  if (v2 > range[1]) {v2 = static_cast<int>(range[1]);}
  if (v3 < range[0]) {v3 = static_cast<int>(range[0]);}
  if (v3 > range[1]) {v3 = static_cast<int>(range[1]);}
  this->MinMax[2]->GetRange(range);
  if (v4 < range[0]) {v4 = static_cast<int>(range[0]);}
  if (v4 > range[1]) {v4 = static_cast<int>(range[1]);}
  if (v5 < range[0]) {v5 = static_cast<int>(range[0]);}
  if (v5 > range[1]) {v5 = static_cast<int>(range[1]);}

  if ( v1 >= v0 )
    {
    this->MinMax[0]->SetMinValue(v0);
    this->MinMax[0]->SetMaxValue(v1);
    }

  if ( v3 >= v2 )
    {
    this->MinMax[1]->SetMinValue(v2);
    this->MinMax[1]->SetMaxValue(v3);
    }

  if ( v5 >= v4 )
    {
    this->MinMax[2]->SetMinValue(v4);
    this->MinMax[2]->SetMaxValue(v5);
    }
  
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                 vtkPVAnimationInterfaceEntry *ai)
{
  vtkKWMenu *cascadeMenu;
  char methodAndArgs[200];

  // Lets create a cascade menu to keep things neat.
  cascadeMenu = vtkKWMenu::New();
  cascadeMenu->SetParent(menu);
  cascadeMenu->Create(this->GetApplication(), "-tearoff 0");
  menu->AddCascade(this->GetTraceName(), cascadeMenu, 0,
                             "Choose a plane of the extent to animate.");  
  // i min
  sprintf(methodAndArgs, "AnimationMenuCallback %s 0", ai->GetTclName());
  cascadeMenu->AddCommand("I Min", this, methodAndArgs, 0, "");
  // i max
  sprintf(methodAndArgs, "AnimationMenuCallback %s 1", ai->GetTclName());
  cascadeMenu->AddCommand("I Max", this, methodAndArgs, 0, "");
  // j min
  sprintf(methodAndArgs, "AnimationMenuCallback %s 2", ai->GetTclName());
  cascadeMenu->AddCommand("J Min", this, methodAndArgs, 0, "");
  // j max
  sprintf(methodAndArgs, "AnimationMenuCallback %s 3", ai->GetTclName());
  cascadeMenu->AddCommand("J Max", this, methodAndArgs, 0, "");
  // k min
  sprintf(methodAndArgs, "AnimationMenuCallback %s 4", ai->GetTclName());
  cascadeMenu->AddCommand("K Min", this, methodAndArgs, 0, "");
  // k min
  sprintf(methodAndArgs, "AnimationMenuCallback %s 5", ai->GetTclName());
  cascadeMenu->AddCommand("K Max", this, methodAndArgs, 0, "");

  cascadeMenu->Delete();
  cascadeMenu = NULL;

  vtkSMProperty *prop = this->GetSMProperty();
  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(prop->GetDomain("extent"));
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai,
                                             int mode)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s) %d", 
      this->GetTclName(), ai->GetTclName(), mode);
    }

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMExtentDomain *dom = 0;
  if (prop)
    {
    dom = vtkSMExtentDomain::SafeDownCast(prop->GetDomain("extent"));
    }
  
  if (!prop || !dom)
    {
    vtkErrorMacro("Error getting extent domain.");
    return;
    }
  
  int exists = 0, val;

  switch (mode)
    {
    case 0:
      ai->SetLabelAndScript("I Min", NULL, this->GetTraceName());
      break;
    case 1:
      ai->SetLabelAndScript("I Max", NULL, this->GetTraceName());
      break;
    case 2:
      ai->SetLabelAndScript("J Min", NULL, this->GetTraceName());
      break;
    case 3:
      ai->SetLabelAndScript("J Max", NULL, this->GetTraceName());
      break;
    case 4:
      ai->SetLabelAndScript("K Min", NULL, this->GetTraceName());
      break;
    case 5:
      ai->SetLabelAndScript("K Max", NULL, this->GetTraceName());
      break;
    default:
      vtkErrorMacro("Bad animation extent");
      return;
    }

  val = dom->GetMinimum(mode/2, exists);
  if (exists)
    {
    ai->SetTimeStart(val);
    }
  val = dom->GetMaximum(mode/2, exists);
  if (exists)
    {
    ai->SetTimeEnd(val);
    }
  ai->SetAnimationElement(mode);
  ai->Update();
}

vtkPVExtentEntry* vtkPVExtentEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVExtentEntry::SafeDownCast(clone);
}

void vtkPVExtentEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVExtentEntry* pvee = vtkPVExtentEntry::SafeDownCast(clone);
  if (pvee)
    {
    pvee->SetLabel(this->Label);
    pvee->UseCellExtent = this->UseCellExtent;

    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvee->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVExtentEntry.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVExtentEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  // Setup the cell_extent.
  if(!element->GetScalarAttribute("use_cell_extent",
                                  &this->UseCellExtent))
    {
    this->UseCellExtent = 0;
    }
  
  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element" << input_menu);
    return 0;
    }
  
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);
  this->PropagateEnableState(this->InputMenu);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->MinMax[cc]);
    }
}

//-----------------------------------------------------------------------------
void vtkPVExtentEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  os << indent << "Label: " << (this->Label ? this->Label : "(none)") << endl;
  os << indent << "Range: " << this->Range[0] << " " << this->Range[1] << endl;
  os << indent << "AnimationAxis: " << this->AnimationAxis << endl;
}
