<?
function error()
{
	// todo make error output dependent on the file type (error image for images, etc)
	exit('Error. <a href="http://dcplusplus.sourceforge.net/">Click here to go back to the main DC++ site.</a>');
}

if (!isset($_SERVER) || !isset($_SERVER['SCRIPT_URL']))
{
	error();
}

@session_start();

if (isset($_POST) && isset($_POST['language']))
{
	$_SESSION['language'] = $_POST['language'];
}

if (isset($_SESSION) && isset($_SESSION['language']))
{
	$language = $_SESSION['language'];
}

else
{
	$language = 'en-US';

	if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE']))
	{
		$langs = explode(',', $_SERVER['HTTP_ACCEPT_LANGUAGE']);
		foreach ($langs as $lang)
		{
			// some languages have an "affinity" argument; remove it
			$pos = strpos($lang, ';');
			if ($pos !== FALSE)
			{
				$lang = substr($lang, 0, $pos);
			}

			// look for an exact match
			if (@is_dir($lang))
			{
				$language = $lang;
				break;
			}

			// ignore the "sub-language" part
			$pos = strpos($lang, '-');
			if ($pos !== FALSE)
			{
				$lang = substr($lang, 0, $pos);
				if (@is_dir($lang))
				{
					$language = $lang;
					break;
				}
			}
		}
	}
}

$name = str_replace('/webhelp/', '', $_SERVER['SCRIPT_URL']); // todo to be correct, one should strrev the last slash etc
$pos = strpos($name, '.');
if ($name == '' || $pos === FALSE)
{
	$name = 'index.html';
	$pos = 5;
}

$output = @file_get_contents("$language/$name");
if ($output === FALSE)
{
	error();
}

$ext = substr($name, $pos + 1);
switch ($ext)
{
case 'bmp':
	$type = 'image/x-ms-bmp';
	break;
case 'css':
	$type = 'text/css';
	break;
case 'ico':
	$type = 'image/vnd.microsoft.icon';
	break;
case 'png':
	$type = 'image/png';
	break;
default:
	$type = 'text/html';
	break;
}
header("Content-Type: $type");

if ($ext == '.html' || $ext == '.htm')
{
	exit($output);
}

$pos = strpos($output, '<body');
if ($pos === FALSE)
{
	exit($output);
}
$pos = strpos($output, '>', $pos);
if ($pos === FALSE)
{
	exit($output);
}
$pos++;

echo substr($output, 0, $pos);
?>
<form method="post" action="<?= $name ?>">
	Language:
	<select name="language">
<?
$dirs = array_diff(@scandir('.'), array('.', '..'));
foreach ($dirs as $dir)
{
	if (!@is_dir($dir))
	{
		continue;
	}

	echo "\t\t<option value=\"$dir\"";
	if ($dir == $language)
	{
		echo ' selected="selected"';
	}
	echo ">$dir</option>\n"; // todo convert language code to actual name
}
?>
	</select>
	<input type="submit" value="Change"/>
</form>
<?= substr($output, $pos); ?>
