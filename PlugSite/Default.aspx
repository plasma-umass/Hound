<%@ Page Language="C#" AutoEventWireup="true"  CodeFile="Default.aspx.cs" Debug="true" Inherits="_Default" %>
<%@ Import Namespace="Saxon.Api" %>

<html xmlns="http://www.w3.org/1999/xhtml" 
      xmlns:svg="http://www.w3.org/2000/svg">
<head runat="server">
    <title>Untitled Page</title>
    <object id="AdobeSVG" classid="clsid:78156a80-c6a1-4bbf-8e6a-3cd390eeb4e2"></object>
    <?import namespace="svg" implementation="#AdobeSVG"?>
    <script type="text/javascript" language="javascript">
      function toggleDiv(divid){
        if(document.getElementById(divid).style.display == 'none') {
          document.getElementById(divid).style.display = 'block';
        } else {
          document.getElementById(divid).style.display = 'none';
        }
      }
    </script>
</head>
<body>
    <form id="form1" runat="server">
    <asp:Label ID="lbl1" runat="server"></asp:Label>
     
    <asp:TextBox id="tb1" runat='server'>
 
    </asp:TextBox>
    </form>
    
    <% GenXML(); %>
</body>
</html>
