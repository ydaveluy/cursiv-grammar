VERSION 5.00
Begin {C932BA85-4374-101B-A56C-00AA003D7352} Picker
   Left            =   120
   Top             =   120
   BeginProperty Font
      Name            =   "Tahoma"
      Size            =   8.25
      Charset         =   0
   EndProperty
   Begin VB.CommandButton OK
      Caption         =   "OK"
      Left            =   240
      Top             =   360
   End
End
Attribute VB_Name = "Picker"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

' Written for this port: a GUID class id and a BeginProperty block, neither
' exercised by an example in the upstream corpus.
Private Sub OK_Click()
    Unload Me
End Sub
