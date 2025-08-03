import re
import sys
from pathlib import Path

# 匹配不在 tr(...) 内、包含中文的字符串字面量
pattern = re.compile(r'(?<!tr\()(".*?[\u4e00-\u9fa5]+.*?")')

# 匹配 new QLabel("...") 中的中文字符串
pattern_label = re.compile(r'(new QLabel\()\s*"([^"]*[\u4e00-\u9fa5]+[^"]*)"\s*(\))')

# 匹配 qDebug() << "中文" 格式
pattern_qdebug = re.compile(r'(qDebug\(\)\s*<<\s*)"([^"]*[\u4e00-\u9fa5]+[^"]*)"')

# 匹配 qWarning() << "中文" 格式
pattern_qwarning = re.compile(r'(qWarning\(\)\s*<<\s*)"([^"]*[\u4e00-\u9fa5]+[^"]*)"')

# 匹配 qInfo() << "中文" 格式
pattern_qinfo = re.compile(r'(qInfo\(\)\s*<<\s*)"([^"]*[\u4e00-\u9fa5]+[^"]*)"')

# 匹配 qCritical() << "中文" 格式
pattern_qcritical = re.compile(r'(qCritical\(\)\s*<<\s*)"([^"]*[\u4e00-\u9fa5]+[^"]*)"')

def wrap_line(line: str) -> str:
    # 首先处理通用的字符串字面量
    line = pattern.sub(lambda m: f'tr({m.group(1)})', line)

    # 处理 new QLabel 构造函数
    line = pattern_label.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}"){m.group(3)}', line)

    # 处理 qDebug() << "中文字符串"
    line = pattern_qdebug.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}")', line)

    # 处理 qWarning() << "中文字符串"
    line = pattern_qwarning.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}")', line)

    # 处理 qInfo() << "中文字符串"
    line = pattern_qinfo.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}")', line)

    # 处理 qCritical() << "中文字符串"
    line = pattern_qcritical.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}")', line)

    return line

def process_file(path: Path) -> bool:
    try:
        text = path.read_text(encoding='utf-8')
        lines = text.splitlines(keepends=True)
        new_text = ''.join(wrap_line(line) for line in lines)

        if new_text != text:
            # 备份原文件
            bak_path = path.with_suffix(path.suffix + '.bak')
            bak_path.write_text(text, encoding='utf-8')
            print(f"  备份文件: {bak_path}")

            # 写入修改后的内容
            path.write_text(new_text, encoding='utf-8')
            return True
    except Exception as e:
        print(f"处理文件 {path} 时出错: {e}")

    return False

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("用法: python3 wrap_tr.py <file_or_directory>")
        print("示例:")
        print("  python3 wrap_tr.py widget.cpp          # 处理单个文件")
        print("  python3 wrap_tr.py /path/to/project    # 处理目录下所有.cpp和.h文件")
        sys.exit(1)

    target = Path(sys.argv[1])
    processed = []

    if target.is_file():
        # 处理单个文件
        if target.suffix in ['.cpp', '.h', '.txt']:
            if process_file(target):
                processed.append(str(target))
        else:
            print(f"警告: {target} 不是支持的文件类型 (.cpp, .h, .txt)")
    elif target.is_dir():
        # 处理目录下所有相关文件
        for pat in ("*.cpp", "*.h"):
            for file in target.rglob(pat):
                if process_file(file):
                    processed.append(str(file))
    else:
        print(f"错误: {target} 不存在")
        sys.exit(1)

    print(f"\n已处理 {len(processed)} 个文件：")
    for f in processed:
        print("  ", f)

    if not processed:
        print("没有文件需要修改。")
