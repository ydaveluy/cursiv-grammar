VERSION 1.0 CLASS
BEGIN
  MultiUse = -1
END
Attribute VB_Name = "Coverage"
Implements ISomeInterface

Public WithEvents Src As Coverage
Global GCount As Integer
Global Const GLimit As Integer = 10

Public Event Changed(ByVal newValue As Long)

Private Sub Coverage_Test()
    Dim result As Long
    Dim short10 As String * 10
    Dim arr() As Integer

    result = Compute(a:=1, b:=2)
    result = Compute(1, , 3)

    If TypeOf Src Is Coverage Then
        RaiseEvent Changed(1)
    End If

    Call AddressOf_User(AddressOf Coverage_Test)

    a = 1: b = 2: c = 3

    On Local Error GoTo Handler
    On result GoTo L1, L2, L3
    On result GoSub S1, S2

    Seek #1, 5
    Lock #1
    Lock #1, 1 To 10
    Unlock #1

    Put #1, 1, short10
    Get #1, 1, short10
    Input #1, result

    LSet short10 = "left"
    RSet short10 = "right"

    SavePicture Nothing, "c:\pic.bmp"
    SendKeys "{ENTER}", True
    SetAttr "c:\file", 1

    GoSub S1

    Exit Sub

L1:
L2:
L3:
S1:
    Return
S2:
    Return

Handler:
    Resume Next
End Sub

Static Sub StaticProcedure()
    Dim [bracketed name] As Long
    [bracketed name] = 1
End Sub

Private Function Compute(a As Long, Optional b As Long, Optional c As Long) As Long
    Compute = a
End Function

Private Sub AddressOf_User(f As Long)
End Sub
