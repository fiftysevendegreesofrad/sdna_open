Module Module1

    Sub TestSDNACom()
        System.Console.WriteLine("hello world")

        'Dim progressbar As Object = CreateObject("sDNAProgressBar.sDNAProgressBar")
        'progressbar.showform()
        'System.Console.WriteLine("showed prog bar directly")
        'System.Console.ReadLine()

        Dim net As Object = CreateObject("sdnacomwrapper.SpatialNet")
        System.Console.WriteLine("got net")
        net.configure(0)
        System.Console.WriteLine("configured net")
        System.Console.ReadLine()

        Dim config As Object = "radii=n;metric=angular"
        Dim calculation As Object = CreateObject("sdnacomwrapper.Calculation")
        calculation.defaultconfig(net)
        'calculation.configure(config, net, Nothing) 'fails due to type errors
        System.Console.WriteLine("config success") ' we get this far if progress bar is disabled in sdnacomwrapper
        Dim names As String()
        calculation.get_output_names(names)
        System.Console.WriteLine(names(0))




        System.Console.ReadLine()
    End Sub

    Sub Main()
        System.Console.WriteLine("hello world")
        System.Console.ReadLine()
    End Sub

End Module
