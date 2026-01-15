#!/usr/bin/env python3
"""
Vietnamese Keyboard Engine Benchmark Script
============================================
So sánh tốc độ gõ tiếng Việt giữa các bộ gõ: UniKey, EVKey, NextKey

Cách sử dụng:
1. Cài đặt dependencies: pip install pyautogui keyboard pyperclip
2. Mở app cần test (Notepad, VS Code, etc.)
3. Bật bộ gõ tiếng Việt cần benchmark (UniKey/EVKey/NextKey)
4. Chạy script: python keyboard_benchmark.py
5. Click vào cửa sổ app trong vòng 5 giây
6. Script sẽ tự động gõ và đo thời gian

Lưu ý: Chạy lần lượt với từng bộ gõ để so sánh kết quả
"""

import time
import sys
import statistics
import argparse
from datetime import datetime

try:
    import pyautogui
    import keyboard
except ImportError:
    print("Cần cài đặt dependencies:")
    print("  pip install pyautogui keyboard")
    sys.exit(1)

# ============================================================
# Đoạn text benchmark tiếng Việt (Telex input)
# ============================================================
# Format: (raw_telex_input, expected_output)
BENCHMARK_TEXTS = {
    "short": {
        "telex": "xin chaof banf ddangg laof gix ",
        "expected": "xin chào bạn đang làm gì ",
        "description": "Câu ngắn cơ bản"
    },
    "medium": {
        "telex": (
            "Buooir sasngs hooms nay troiws ddepj quas. "
            "Toois ddi daoj mootj vongs quanh congs vieens "
            "vaaf thays nhuwxng boong hoa nowr rootj. "
            "Muaf xuaan ddax deens roofif. "
        ),
        "expected": (
            "Buổi sáng hôm nay trời đẹp quá. "
            "Tôi đi dạo một vòng quanh công viên "
            "và thấy những bông hoa nở rộ. "
            "Mùa xuân đã đến rồi. "
        ),
        "description": "Đoạn văn trung bình"
    },
    "long": {
        "telex": (
            "Vieejt Nam laf mootj ddaats nuwowcs coosf kinhsr nhieefuf traams nams lichj suwrt. "
            "Tuwf thowif Hunfg Vuwowng ddeeens nafy, daans tooojc vieejt ddax duwngj leebn mootj neefn vans hoaas ddoojc ddaaos. "
            "Quees huwowng toois coosf nhuwxng ddoofngs luasf baast ngaats, nhuwxng doofng soongs hieeefn hoaaf, "
            "vaaf nhuwxng nguwowif con gasnf guix viwsf langj. "
            "Toois yeebu queeb huwowng toois, nowi ddax sinh ra vaaf nuwois duwowxng toois. "
            "Dusf ddi xas baooo lauau, toois vaaxn luoons nhows veef nowi aays. "
        ),
        "expected": (
            "Việt Nam là một đất nước có kinh nhiều trăm năm lịch sử. "
            "Từ thời Hùng Vương đến nay, dân tộc việt đã dựng lên một nền văn hoá độc đáo. "
            "Quê hương tôi có những đồng lúa bạt ngạt, những dòng sông hiền hoà, "
            "và những người con gắn gũi vị làng. "
            "Tôi yêu quê hương tôi, nơi đã sinh ra và nuôi dưỡng tôi. "
            "Dù đi xa bao lâu, tôi vẫn luôn nhớ về nơi ấy. "
        ),
        "description": "Đoạn văn dài về Việt Nam"
    },
    "special_chars": {
        "telex": (
            "Aawn aws awj awx awr "  # ă ằ ặ ẵ ắ
            "Aan aas aaf aax aar "   # â ấ ầ ẩ ẫ  
            "Een ees eef eex eer "   # ê ế ề ể ễ
            "Oon oos oof oox oor "   # ô ố ồ ổ ỗ
            "Own ows owf owx owr "   # ơ ớ ờ ở ỡ
            "Uwn uws uwf uwx uwr "   # ư ứ ừ ử ữ
            "dda dde ddi ddo ddu "   # đa đe đi đo đu
        ),
        "expected": "Các ký tự đặc biệt tiếng Việt",
        "description": "Test ký tự đặc biệt"
    }
}

# ============================================================
# Benchmark Functions
# ============================================================

def countdown(seconds: int):
    """Đếm ngược trước khi bắt đầu benchmark"""
    print(f"\n⏳ Chuẩn bị trong {seconds} giây...")
    print("   Hãy click vào cửa sổ app cần test!")
    for i in range(seconds, 0, -1):
        print(f"   {i}...", end="\r")
        time.sleep(1)
    print("   🚀 Bắt đầu!      ")


def type_and_measure(text: str, delay_per_char: float = 0.01) -> dict:
    """
    Gõ đoạn text và đo thời gian
    
    Args:
        text: Đoạn text cần gõ (Telex raw input)
        delay_per_char: Delay giữa các ký tự (giây)
    
    Returns:
        dict với các metrics
    """
    char_count = len(text)
    
    # Warm up - đợi focus ổn định
    time.sleep(0.1)
    
    # Đo thời gian bắt đầu
    start_time = time.perf_counter_ns()
    
    # Gõ từng ký tự
    pyautogui.write(text, interval=delay_per_char)
    
    # Đo thời gian kết thúc
    end_time = time.perf_counter_ns()
    
    # Tính toán metrics
    total_time_ms = (end_time - start_time) / 1_000_000
    total_time_s = total_time_ms / 1000
    chars_per_second = char_count / total_time_s if total_time_s > 0 else 0
    avg_latency_ms = total_time_ms / char_count if char_count > 0 else 0
    
    return {
        "char_count": char_count,
        "total_time_ms": total_time_ms,
        "chars_per_second": chars_per_second,
        "avg_latency_ms": avg_latency_ms,
        "delay_per_char_ms": delay_per_char * 1000
    }


def run_benchmark(text_key: str = "medium", runs: int = 3, delay: float = 0.01) -> list:
    """
    Chạy benchmark nhiều lần và trả về kết quả
    
    Args:
        text_key: Key của đoạn text trong BENCHMARK_TEXTS
        runs: Số lần chạy
        delay: Delay giữa các ký tự
    
    Returns:
        List các kết quả
    """
    if text_key not in BENCHMARK_TEXTS:
        print(f"❌ Không tìm thấy text '{text_key}'")
        print(f"   Các option: {list(BENCHMARK_TEXTS.keys())}")
        return []
    
    text_data = BENCHMARK_TEXTS[text_key]
    telex_input = text_data["telex"]
    
    print(f"\n📝 Benchmark: {text_data['description']}")
    print(f"   Đoạn text ({len(telex_input)} ký tự Telex)")
    print(f"   Số lần chạy: {runs}")
    
    results = []
    
    for i in range(runs):
        print(f"\n--- Lần chạy {i + 1}/{runs} ---")
        
        # Chờ Enter để bắt đầu (cho phép user focus app)
        if i == 0:
            countdown(5)
        else:
            print("   Đang chạy...")
            time.sleep(1)
        
        result = type_and_measure(telex_input, delay)
        results.append(result)
        
        print(f"   ✅ Hoàn thành: {result['total_time_ms']:.2f}ms")
        print(f"   📊 Tốc độ: {result['chars_per_second']:.1f} chars/s")
        
        # Thêm newline sau mỗi lần chạy
        pyautogui.press('enter')
        pyautogui.press('enter')
        time.sleep(0.5)
    
    return results


def print_summary(results: list, engine_name: str = "Unknown"):
    """In tổng kết benchmark"""
    if not results:
        return
    
    times = [r["total_time_ms"] for r in results]
    speeds = [r["chars_per_second"] for r in results]
    latencies = [r["avg_latency_ms"] for r in results]
    
    print("\n" + "=" * 60)
    print(f"📊 KẾT QUẢ BENCHMARK - {engine_name.upper()}")
    print("=" * 60)
    print(f"  Số lần chạy: {len(results)}")
    print(f"  Số ký tự mỗi lần: {results[0]['char_count']}")
    print()
    print("  ⏱️  Thời gian tổng:")
    print(f"      Min: {min(times):.2f}ms")
    print(f"      Max: {max(times):.2f}ms")
    print(f"      Avg: {statistics.mean(times):.2f}ms")
    if len(times) > 1:
        print(f"      StdDev: {statistics.stdev(times):.2f}ms")
    print()
    print("  🚀 Tốc độ (chars/second):")
    print(f"      Min: {min(speeds):.1f}")
    print(f"      Max: {max(speeds):.1f}")
    print(f"      Avg: {statistics.mean(speeds):.1f}")
    print()
    print("  ⚡ Latency trung bình/ký tự:")
    print(f"      Avg: {statistics.mean(latencies):.3f}ms")
    print("=" * 60)


def save_results(results: list, engine_name: str):
    """Lưu kết quả ra file"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"benchmark_{engine_name}_{timestamp}.txt"
    
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"Benchmark Results - {engine_name}\n")
        f.write(f"Timestamp: {datetime.now().isoformat()}\n")
        f.write("=" * 50 + "\n\n")
        
        times = [r["total_time_ms"] for r in results]
        speeds = [r["chars_per_second"] for r in results]
        
        f.write(f"Runs: {len(results)}\n")
        f.write(f"Chars per run: {results[0]['char_count']}\n\n")
        f.write(f"Total time (avg): {statistics.mean(times):.2f}ms\n")
        f.write(f"Speed (avg): {statistics.mean(speeds):.1f} chars/s\n")
    
    print(f"\n💾 Đã lưu kết quả: {filename}")


# ============================================================
# Main
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="Benchmark Vietnamese Keyboard Engines",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ví dụ:
  python keyboard_benchmark.py --engine nextkey --text medium --runs 3
  python keyboard_benchmark.py --engine unikey --text long --runs 5
  python keyboard_benchmark.py --list-texts
        """
    )
    
    parser.add_argument(
        "--engine", "-e",
        type=str,
        default="unknown",
        help="Tên bộ gõ đang test (nextkey/unikey/evkey)"
    )
    
    parser.add_argument(
        "--text", "-t",
        type=str,
        default="medium",
        choices=list(BENCHMARK_TEXTS.keys()),
        help="Đoạn text để benchmark (default: medium)"
    )
    
    parser.add_argument(
        "--runs", "-r",
        type=int,
        default=3,
        help="Số lần chạy benchmark (default: 3)"
    )
    
    parser.add_argument(
        "--delay", "-d",
        type=float,
        default=0.01,
        help="Delay giữa các ký tự (giây, default: 0.01)"
    )
    
    parser.add_argument(
        "--save", "-s",
        action="store_true",
        help="Lưu kết quả ra file"
    )
    
    parser.add_argument(
        "--list-texts",
        action="store_true",
        help="Liệt kê các đoạn text có sẵn"
    )
    
    args = parser.parse_args()
    
    # Liệt kê texts
    if args.list_texts:
        print("\n📝 Các đoạn text có sẵn:")
        for key, data in BENCHMARK_TEXTS.items():
            print(f"\n  {key}:")
            print(f"    Description: {data['description']}")
            print(f"    Telex: {data['telex'][:50]}...")
            print(f"    Length: {len(data['telex'])} chars")
        return
    
    # Banner
    print("\n" + "=" * 60)
    print("🇻🇳 VIETNAMESE KEYBOARD ENGINE BENCHMARK")
    print("=" * 60)
    print(f"  Engine: {args.engine.upper()}")
    print(f"  Text: {args.text}")
    print(f"  Runs: {args.runs}")
    print(f"  Delay: {args.delay * 1000:.1f}ms per char")
    print()
    print("⚠️  LƯU Ý:")
    print("  1. Đảm bảo bộ gõ tiếng Việt đã BẬT")
    print("  2. Click vào cửa sổ app trước khi countdown kết thúc")
    print("  3. KHÔNG di chuyển chuột hay gõ phím trong lúc test")
    print()
    
    # Xác nhận
    input("Nhấn Enter để bắt đầu...")
    
    # Chạy benchmark
    results = run_benchmark(args.text, args.runs, args.delay)
    
    if results:
        print_summary(results, args.engine)
        
        if args.save:
            save_results(results, args.engine)
    
    print("\n✅ Benchmark hoàn thành!")
    print("   Chạy lại với bộ gõ khác để so sánh.")


if __name__ == "__main__":
    main()
