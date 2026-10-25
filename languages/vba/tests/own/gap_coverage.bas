Attribute VB_Name = "GapCoverage"
' Written for this port: exercises constructs no example in the upstream corpus uses --
' Not part of the upstream corpus.
Option Explicit
Option Compare Binary
Option Base 1
Option Private Module

DefInt A-C
DefStr X, Y-Z

Public Declare PtrSafe Function GetTickCount Lib "kernel32" () As Long
Private Declare Sub Sleep Lib "kernel32" Alias "SleepEx" (ByVal dwMilliseconds As Long)

Public Enum Color
    Red
    Green = 5
    Blue
End Enum

Public Type Point
    X As Long
    Y As Long
    Label As String * 10
End Type

Public Event Changed(ByVal newValue As Variant)

Private WithEvents Watcher As Point

Public Function Sum(ParamArray Values() As Variant) As Double
    Dim i As Integer
    For i = LBound(Values) To UBound(Values)
        Sum = Sum + Values(i)
    Next i
End Function

Public Sub Demo()
    Dim p As Point
    Dim v As Variant
    Dim n As Long
    Dim handler As Long

    On Error GoTo Handler

    p.X = 1
    p.Y = 2

    If TypeOf Me Is Point Then
        Debug.Print "typed"
    End If

    handler = 0
    LSet p.Label = "hi"
    RSet p.Label = "lo"

    Dim s As String
    s = "hello world"
    Mid(s, 1, 5) = "HELLO"

    n = 1
    On Error Resume Next
    GoSub Worker
    On Error GoTo 0

    RaiseEvent Changed(42)

    Dim d As Date
    d = #12/31/2020 11:59:00 PM#
    Dim f As Double
    f = 1.5E+10
    Dim g As Double
    g = .5
    Dim guidLike As Variant
    guidLike = Nothing
    v = Null
    v = Empty

    Dim ff As Integer
    ff = FreeFile
    Open "test.txt" For Random Access Read Write Lock Read Write As #ff Len = 128
    Seek #ff, 1
    Lock #ff, 1 To 10
    Unlock #ff, 1 To 10
    Line Input #ff, s
    Width #ff, 80
    Write #ff, n, s
    Put #ff, 1, p
    Get #ff, 1, p
    Close #ff

    Exit Sub

Worker:
    n = n + 1
    Return

Handler:
    Resume Next
End Sub
