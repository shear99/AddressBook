import boto3
import numpy as np
import cv2
import os
from datetime import datetime
import random
import string

# S3 클라이언트 초기화
s3 = boto3.client('s3')
bucket_name = 'contact-photo-bucket-001'

def generate_random_color():
    """랜덤한 색상 생성"""
    return (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))

def generate_random_pattern():
    """랜덤한 패턴 생성"""
    pattern = np.zeros((512, 512, 3), dtype=np.uint8)
    
    # 배경색 설정
    bg_color = generate_random_color()
    pattern[:] = bg_color
    
    # 패턴 생성
    pattern_type = random.choice(['circles', 'lines', 'squares'])
    
    if pattern_type == 'circles':
        for _ in range(5):
            center = (random.randint(100, 412), random.randint(100, 412))
            radius = random.randint(20, 100)
            color = generate_random_color()
            cv2.circle(pattern, center, radius, color, -1)
    
    elif pattern_type == 'lines':
        for _ in range(5):
            pt1 = (random.randint(0, 512), random.randint(0, 512))
            pt2 = (random.randint(0, 512), random.randint(0, 512))
            color = generate_random_color()
            thickness = random.randint(2, 10)
            cv2.line(pattern, pt1, pt2, color, thickness)
    
    else:  # squares
        for _ in range(5):
            x = random.randint(0, 412)
            y = random.randint(0, 412)
            size = random.randint(20, 100)
            color = generate_random_color()
            cv2.rectangle(pattern, (x, y), (x+size, y+size), color, -1)
    
    return pattern

def add_number_to_image(image, number):
    """이미지에 번호 추가"""
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 2
    font_thickness = 4
    text = f"#{number}"
    
    # 텍스트 크기 계산
    (text_width, text_height), _ = cv2.getTextSize(text, font, font_scale, font_thickness)
    
    # 텍스트 위치 계산 (중앙)
    x = (512 - text_width) // 2
    y = (512 + text_height) // 2
    
    # 텍스트 배경 추가
    cv2.rectangle(image, (x-10, y-text_height-10), (x+text_width+10, y+10), (255, 255, 255), -1)
    
    # 텍스트 추가
    cv2.putText(image, text, (x, y), font, font_scale, (0, 0, 0), font_thickness)
    
    return image

def generate_and_upload_images(num_images=30):
    """더미 이미지 생성 및 S3 업로드"""
    for i in range(1, num_images + 1):
        # 이미지 생성
        image = generate_random_pattern()
        image = add_number_to_image(image, i)
        
        # 이미지를 메모리에서 JPEG로 인코딩
        _, buffer = cv2.imencode('.jpg', image)
        
        # S3 키 생성
        timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
        s3_key = f'processed/dummy_{i}_{timestamp}.jpg'
        
        # S3에 업로드
        s3.put_object(
            Bucket=bucket_name,
            Key=s3_key,
            Body=buffer.tobytes(),
            ContentType='image/jpeg'
        )
        
        print(f"Uploaded image {i} to {s3_key}")
        
        # 로컬에도 저장 (선택적)
        cv2.imwrite(f'dummy_{i}.jpg', image)

if __name__ == "__main__":
    generate_and_upload_images() 