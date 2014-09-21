declare namespace my = "urn:foo";
declare namespace svg = "http://www.w3.org/2000/svg";
declare namespace xhtml = "http://www.w3.org/1999/xhtml";

declare function my:graphSVG_CDF($site) {
for $x in $site/block
let $tpop := sum($site/block/population),
$tstale := max($site/block/stale),
$ct := count($site/block),
$pos := count($site/block[number(stale) <= number($x/stale)])
order by number($x/stale)
return string-join(("L", string(300 * number($x/stale) div $tstale), string(200-200 * number($pos) div $ct)), " " )
(: return string-join((($x/stale), string($i)), " ") :)
};

declare function my:graphSVG_PDF($site) {
for $x at $i in $site/block
let $tpop := sum($site/block/population),
    $mstale := max($site/block/stale)
order by number($x/stale)
return string-join(("M", string(300 * number($x/stale) div $mstale), "200",
	"l", "0", string(-200 * number($x/population) div $tpop)), " " )
};

declare function my:scoreSeq($site) {
for $x in $site/block
return data($x/stale)*data($x/population)
};

declare function my:stackDesc($site) {

if(count($site/frame) > 0) then
(
for $x in $site/frame
return   <tr>
    <td>
      {data($x/sym)}
    </td>
    <td>{data($x/eip)}</td>
  </tr>
)
else

<tr>
  <td>
    NONE
  </td>
</tr>
};

declare function my:siteCount($asite,$tsite) {
  count(for $x in $asite/block/touchsite/@hash where $x = $tsite return $x)
};

declare function my:touchList($asite,$tsites) {
  for $x at $i in $tsites
  let $ct := my:siteCount($asite,$x)
  order by $ct descending
  return 
  <li>
    <a onclick="toggleDiv('div{$asite/@hash}{$x}');">{$x}: {$ct}</a>
    <div id="div{$asite/@hash}{$x}" style="vertical-align:top; display:none;">
      <table style="vertical-align:top;">
        {my:stackDesc(($asite/block/touchsite[@hash = $x])[1])}
      </table>
    </div>
  </li>
};

declare function my:main($xml) {
for $x in $xml/heap/allocsite
let $score := sum(my:scoreSeq($x)),
    $minstale := min($x/block/stale),
    $maxstale := max($x/block/stale)
where max($x/block/stale) > 0
order by $score descending
return
<p>
  {data($x/@hash)}: <br/>
  Total population: {sum($x/block/population)}<br/>
  Max stale: {max($x/block/stale)}<br/>
  Blocks: {count($x/block)}<br/>
  Distinct touchsite count: {count(distinct-values($x/block/touchsite/@hash))}<br/>
  <i>Leakage</i>: {$score}<br/>
  <table>
    <tr>
      <th>Staleness Profile (per object)</th>
      <th>Alloc callsite</th>
      <th>Touchsites</th>
    </tr>
    <tr>
      <td style="vertical-align:top;">
        <svg:svg width="300" height="200">
          <svg:line x1="0" y1="0" x2="0" y2="200" style="stroke:rgb(0,0,0);stroke-width:3;"/>
          <svg:line x1="0" y1="200" x2="300" y2="200" style="stroke:rgb(0,0,0);stroke-width:3;"/>
          <svg:path d="M {$minstale div $maxstale} 200 {my:graphSVG_CDF($x)}" style="stroke:rgb(224,0,0);stroke-width:3;fill:rgb(255,255,255)" />
          <!-- <svg:path d="M 0 200 {my:graphSVG_PDF($x)}" style="stroke:rgb(224,0,0);stroke-width:3;fill:rgb(255,255,255)" /> -->
        </svg:svg>
      </td>
      <td style="vertical-align:top;">
        <table>
          {my:stackDesc($x/site)}
        </table>
      </td>
      <td style="vertical-align:top;">
        <ul>
          {my:touchList($x,distinct-values($x/block/touchsite/@hash))}
        </ul>
      </td>
    </tr>
  </table>
</p>
};

<html>
<head runat="server">
    <title>Untitled Page</title>
    <script type="text/javascript" language="javascript">
{"function toggleDiv(divid){
        if(document.getElementById(divid).style.display == 'none') {
          document.getElementById(divid).style.display = 'block';
        } else {
          document.getElementById(divid).style.display = 'none';
        }
      }"}
    </script>
</head>
<body>{
my:main(.)
}</body></html>
