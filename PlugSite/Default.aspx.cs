using System;
using System.Data;
using System.Configuration;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;
using System.IO;
using Saxon.Api;
using System.Xml;
using System.Xml.Xsl;

public partial class _Default : System.Web.UI.Page 
{
    protected void Page_Load(object sender, EventArgs e)
    {

    }

    public void GenXML()
    {
        String sourceUri = Server.MapPath("5648.xml");
        String xqUri = Server.MapPath("graph.xq");

        using (FileStream sXml = File.OpenRead(sourceUri))
        {
            using (FileStream sXq = File.OpenRead(xqUri))
            {
                Processor processor = new Processor();
                XQueryCompiler compiler = processor.NewXQueryCompiler();
                compiler.BaseUri = sourceUri;
                XQueryExecutable exp = compiler.Compile(sXq);
                XQueryEvaluator eval = exp.Load();

                DocumentBuilder loader = processor.NewDocumentBuilder();
                loader.BaseUri = new Uri(sourceUri);
                XdmNode indoc = loader.Build(new FileStream(sourceUri, FileMode.Open, FileAccess.Read));

                eval.ContextItem = indoc;
                Serializer qout = new Serializer();
                qout.SetOutputProperty(Serializer.METHOD, "xml");
                qout.SetOutputProperty(Serializer.INDENT, "yes");
                qout.SetOutputProperty(Serializer.SAXON_INDENT_SPACES, "1");
                qout.SetOutputWriter(Response.Output);
                eval.Run(qout);
            }
        }
    }
}
