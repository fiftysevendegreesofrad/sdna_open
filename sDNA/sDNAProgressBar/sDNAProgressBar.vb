'//sDNA software for spatial network analysis 
'//Copyright (C) 2011-2019 Cardiff University

'//This program is free software: you can redistribute it and/or modify
'//it under the terms of the GNU General Public License as published by
'//the Free Software Foundation, either version 3 of the License, or
'//(at your option) any later version.

'//This program is distributed in the hope that it will be useful,
'//but WITHOUT ANY WARRANTY; without even the implied warranty of
'//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'//GNU General Public License for more details.

'//You should have received a copy of the GNU General Public License
'//along with this program.  If not, see <https://www.gnu.org/licenses/>.

<ComClass(sDNAProgressBar.ClassId, sDNAProgressBar.InterfaceId, sDNAProgressBar.EventsId)> _
Public Class sDNAProgressBar

#Region "COM GUIDs"
    ' These  GUIDs provide the COM identity for this class 
    ' and its COM interfaces. If you change them, existing 
    ' clients will no longer be able to access the class.
    Public Const ClassId As String = "46783a4a-5ca9-4930-addb-cde91a377c02"
    Public Const InterfaceId As String = "e2ad7298-7339-45fb-aad1-cb82188bf067"
    Public Const EventsId As String = "b7e10cdf-dc2a-4052-9b0f-f6f9d5a1f0ac"
#End Region

    ' A creatable COM class must have a Public Sub New() 
    ' with no parameters, otherwise, the class will not be 
    ' registered in the COM registry and cannot be created 
    ' via CreateObject.
    Public Sub New()
        MyBase.New()
    End Sub

    Private f1 As sDNAProgressBarForm = Nothing

    Public Sub ensure_visible()
        If f1 Is Nothing Then
            f1 = New sDNAProgressBarForm()
            f1.Show()
            f1.Refresh()
        End If
    End Sub

    Public Sub set_to(ByVal d As Double)
        ensure_visible()
        f1.ProgressBar1.Value = d
        f1.Label1.Text = d.ToString(".0") + "% complete"
        f1.Update()
    End Sub

    Public Sub append_text(ByVal s As String)
        ensure_visible()
        f1.TextBox1.AppendText(s.Replace(vbLf, vbCrLf) + vbNewLine)
        f1.TextBox1.Refresh()
        f1.Update()
    End Sub

    Public Sub allow_close()
        If Not f1 Is Nothing Then
            f1.Button1.Enabled = True
        End If
    End Sub

End Class


