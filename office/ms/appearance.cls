VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True'
END

'Ϊÿ��sheet ��������, Ҳ����ͳһ����'
Sub setFont()
On Error Resume Next
For Each s In Worksheets
    s.Cells.Font.Name = "Times New Roman"
Next
End Sub
