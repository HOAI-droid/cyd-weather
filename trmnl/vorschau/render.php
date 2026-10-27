<?php
// Vorschau-Renderer: bildet Plugin::render() und resolveLiquidVariables() von LaraPaper nach
// (gleiche Liquid-Engine keepsuit/liquid 0.12.1, gleiche Filter und Vorverarbeitung).
// Aufruf: php render.php <src-ordner> <size> <daten.json> <zeit-utc> [config-json]
require __DIR__.'/vendor/autoload.php';

use App\Liquid\FileSystems\InlineTemplatesFileSystem;
use App\Liquid\Filters\{Data, Date, Localization, Numbers, StringMarkup, Uniqueness, StandardFilters};
use App\Liquid\Tags\{PluginRenderTag, TemplateTag};
use App\Liquid\Utils\ExpressionUtils;
use Keepsuit\Liquid\Extensions\StandardExtension;
use Symfony\Component\Yaml\Yaml;

date_default_timezone_set('UTC');
[$_, $src, $size, $dataFile, $ts] = $argv;
$override = json_decode($argv[5] ?? '{}', true) ?: [];

$settings = Yaml::parseFile("$src/settings.yml");
$config = [];
foreach ($settings['custom_fields'] ?? [] as $f) {
    if (isset($f['keyname'], $f['default'])) $config[$f['keyname']] = (string) $f['default'];
}
$config = array_merge($config, $override);

function prep(string $t): string {
    $t = preg_replace_callback('/date:\s*(["\'])([^"\']+)\1/', fn($m) => 'date: '.$m[1].ExpressionUtils::strftimeToPhpFormat($m[2]).$m[1], $t);
    $t = preg_replace_callback('/l_date:\s*(["\'])([^"\']+)\1/', fn($m) => 'l_date: '.$m[1].ExpressionUtils::strftimeToPhpFormat($m[2]).$m[1], $t);
    $t = preg_replace('/{%\s*render\s+([^}]+?)\s+with\s+/i', '{% render $1, ', $t);
    return preg_replace_callback('/{%\s*for\s+(\w+)\s+in\s+([^|%}]+)\s*\|\s*([^%}]+)%}/', function ($m) {
        $v = '_temp_'.uniqid();
        return "{% assign $v = ".trim($m[2]).' | '.trim($m[3])." %}{% for ".trim($m[1])." in $v %}";
    }, $t);
}

// Abruf-Adressen auflösen (wie resolveLiquidVariables: nur Standardfilter, Felder auf oberster Ebene)
$env = new \Keepsuit\Liquid\Environment(extensions: [new StandardExtension()]);
$env->filterRegistry->register(StandardFilters::class);
$urls = $env->parseString($settings['polling_url'] ?? '')->render($env->newRenderContext(data: $config));
fwrite(STDERR, "Abruf-Adressen:\n$urls\n");

$data = json_decode(file_get_contents($dataFile), true);
$markup = (file_exists("$src/shared.liquid") ? file_get_contents("$src/shared.liquid")."\n" : '').file_get_contents("$src/$size.liquid");
$markup = preg_match('/<div\s+class=(["\'])[^"\']*\bview\s+view--[^"\']*\1\s*>/', $markup)
    ? $markup : '<div class="view view--{{ size }}">'."\n".$markup."\n</div>";

$env = new \Keepsuit\Liquid\Environment(fileSystem: new InlineTemplatesFileSystem(), extensions: [new StandardExtension()]);
foreach ([Data::class, Date::class, Localization::class, Numbers::class, StringMarkup::class, Uniqueness::class] as $c) {
    $env->filterRegistry->register($c);
}
$env->tagRegistry->register(TemplateTag::class);
$env->tagRegistry->register(PluginRenderTag::class);
$context = [
    'size' => $size, 'data' => $data, 'config' => $config, ...$data,
    'trmnl' => [
        'system' => ['timestamp_utc' => (int) $ts],
        'user' => ['utc_offset' => '3600', 'name' => 'Vorschau', 'locale' => 'en', 'time_zone_iana' => 'Europe/Berlin'],
        'device' => ['width' => 800, 'height' => 480, 'bit_depth' => 1],
        'plugin_settings' => ['instance_name' => $settings['name'], 'custom_fields_values' => $config,
            'refresh_interval_minutes' => $settings['refresh_interval'] ?? 15],
    ],
];
echo $env->parseString(prep($markup))->render($env->newRenderContext(data: $context));
