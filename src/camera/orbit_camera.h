#pragma once

#include "glm/glm.hpp"

/**
 * @brief オービット型カメラ
 *
 * ターゲット点を中心に、
 * - 左右ドラッグで回転（yaw / pitch）
 * - 中ドラッグで平行移動（pan）
 * - ホイールで距離変更（zoom）
 *
 * DCCツール（Blender / Maya）に近い操作感を想定。
 * 座標系は右手系、Y-up。
 */
class OrbitCamera
{
public:
    /**
     * @brief デフォルトコンストラクタ
     *
     * 初期状態：
     * - target = (0,0,0)
     * - yaw = 0
     * - pitch = 0
     * - distance = 5
     */
    OrbitCamera();

    // ===== Input handling =====

    /**
     * @brief カメラをターゲット中心に回転させる
     *
     * @param dx マウスX移動量（ピクセル）
     * @param dy マウスY移動量（ピクセル）
     *
     * yaw / pitch を更新する。
     * pitch は上下反転防止のため制限される。
     */
    void orbit(float dx, float dy);

    /**
     * @brief カメラ基準で平行移動（パン）
     *
     * @param dxPixels マウスX移動量（ピクセル）
     * @param dyPixels マウスY移動量（ピクセル）
     * @param fbW フレームバッファ幅（ピクセル）
     * @param fbH フレームバッファ高さ（ピクセル）
     *
     * 画面空間の移動量を、距離・FOVに基づいて
     * ワールド空間の移動量へ変換する。
     */
    void pan(float dxPixels, float dyPixels, int fbW, int fbH);

    /**
     * @brief カメラ距離を変更する
     *
     * @param wheel マウスホイール入力量
     *
     * ターゲットとの距離を指数的に増減する。
     * 最小・最大距離は内部で制限される。
     */
    void zoom(float wheel);

    // ===== Matrices =====

    /**
     * @brief View 行列を取得する
     * 
     * @return Mat4 View 行列
     *
     * eye / target / up(0,1,0) から
     * 右手系 LookAt 行列を生成する。
     */
    glm::mat4 viewMatrix() const;

    // ===== Debug / UI =====

    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float distance() const { return m_distance; }
    glm::vec3 target() const { return m_target; }

private:
    float m_yaw = 0.0f;        ///< 水平方向回転角（ラジアン）
    float m_pitch = 0.0f;      ///< 垂直方向回転角（ラジアン）
    float m_distance = 5.0f;   ///< ターゲットからの距離
    glm::vec3 m_target{ 0.0f, 0.0f, 0.0f }; ///< 注視点（ワールド座標）

    /**
     * @brief カメラのワールド座標位置を計算する
     *
     * yaw / pitch / distance / target から算出される。
     */
    glm::vec3 eyePosition() const;
};
