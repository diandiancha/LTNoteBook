import re
import sys
from pathlib import Path

# 匹配不在 tr(...) 内、包含中文的字符串字面量
pattern = re.compile(r'(?<!tr\()(".*?[\u4e00-\u9fa5]+.*?")')

def wrap_line(line: str) -> str:
    return pattern.sub(lambda m: f'tr({m.group(1)})', line)

def process_file(path: Path) -> bool:
    text = path.read_text(encoding='utf-8')
    lines = text.splitlines(keepends=True)
    new_text = ''.join(wrap_line(line) for line in lines)
    if new_text != text:
        # 备份原文件
        bak_path = path.with_suffix(path.suffix + '.bak')
        bak_path.write_text(text, encoding='utf-8')
        # 写入修改后的内容
        path.write_text(new_text, encoding='utf-8')
        return True
    return False

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("用法: python3 wrap_tr.py <project_root>")
        sys.exit(1)
    root = Path(sys.argv[1])
    processed = []
    for pat in ("*.cpp", "*.h"):
        for file in root.rglob(pat):
            if process_file(file):
                processed.append(str(file))
    print(f"已处理 {len(processed)} 个文件：")
    for f in processed:
        print("  ", f)

        # 额外匹配 new QLabel("…")
        pattern_label = re.compile(r'(new QLabel\()\s*"([^"]*[\u4e00-\u9fa5]+[^"]*)"\s*(,)')
        def wrap_line(line):
            line = pattern1.sub(...)    # 之前的 tr() 包装
            line = pattern2.sub(...)    # setText 包装
            line = pattern_label.sub(lambda m: f'{m.group(1)}tr("{m.group(2)}"){m.group(3)}', line)
            return line

