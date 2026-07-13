# Doom MAP01 Integration — Progress Tracking

## Ітерація 1. Завантаження та відображення геометрії

**Мета:** MAP01 завантажується, видно геометрію, камера всередині карти.

| Задача | Статус | Коміт |
|--------|--------|-------|
| Material.h — структура матеріалу | ✅ | 72453b2 |
| Triangle: додано UV, materialIndex | ✅ | 72453b2 |
| Mesh: матеріали, textured vertex format | ✅ | 72453b2 |
| OBJ-парсер: v, vt, vn, f v/vt/vn, mtllib, usemtl | ✅ | 72453b2 |
| MTL-парсер: newmtl, map_Kd, Kd | ✅ | 72453b2 |
| OpenGL textured render path | ✅ | 72453b2 |
| Стара карта map1.obj працює | ✅ | 72453b2 |
| Підключити MAP01 до гри | ✅ | поточний |
| YZ-сумісність (RotationX) | ✅ | поточний |
| Відображення геометрії | 🔄 | тестування |
| Правильний масштаб | 🔄 | тестування |
| Правильний winding | 🔄 | тестування |

## Ітерація 2. Стартова позиція та огляд

**Мета:** Гравець стоїть на підлозі, може озирнутися.

| Задача | Статус |
|--------|--------|
| Підібрати spawn position | ⏳ |
| Гравець на підлозі (не в стіні) | ⏳ |
| Можна озирнутися мишкою | ⏳ |
| Кнопка повернення на старт | ⏳ |

## Ітерація 3. Колізії

**Мета:** Гравець не провалюється крізь підлогу, не проходить крізь стіни.

| Задача | Статус |
|--------|--------|
| Перевірити поточну колізію на MAP01 | ⏳ |
| Налаштувати collider | ⏳ |
| Ходьба сходинками | ⏳ |
| Ковзання вздовж стін | ⏳ |

## Ітерація 4. Оптимізація

| Задача | Статус |
|--------|--------|
| Frustum culling | ⏳ |
| Spatial grid для статики | ⏳ |
| Кешування | ⏳ |

## Поточні проблеми

- (додавати після тестування)

## Файли, змінені в Iteration 1

- `engine/Material.h` — **NEW**
- `engine/Triangle.h`, `engine/Triangle.cpp`
- `engine/Mesh.h`, `engine/Mesh.cpp`
- `engine/utils/ResourceManager.h`, `engine/utils/ResourceManager.cpp`
- `engine/io/Screen.h`, `engine/io/Screen.cpp`
- `engine/Engine.cpp`
- `engine/World.h`, `engine/World.cpp`
- `ShooterConsts.h`
- `Shooter.cpp`
